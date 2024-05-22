/* -*- P4_16 -*- */

/*
 * https://github.com/P4-Research/p4-demos/blob/master/vxlan/vxlan.p4app/vxlan.p4
*/
  
#include <core.p4>
#include <v1model.p4>

#define UDP_PORT_VXLAN 4789
#define UDP_PROTO 17
#define TCP_PROTO 6
#define IPV4_ETHTYPE 0x800

#define ETH_HDR_SIZE 14
#define IPV4_HDR_SIZE 20
#define UDP_HDR_SIZE 8
#define VXLAN_HDR_SIZE 8
#define IP_VERSION_4 4
#define IPV4_MIN_IHL 5

register<bit<32>>(4) reg_segment;

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> destPort;
    bit<16> length;
    bit<16> checksum;
}

header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved_2;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct headers {
    @name("ethernet")
    ethernet_t      ethernet;
    @name("ipv4")
    ipv4_t          ipv4;
    @name("udp")
    udp_t           udp;
    @name("vxlan")
    vxlan_t         vxlan;

    ethernet_t      inner_ethernet;
    ipv4_t          inner_ipv4;
    tcp_t           inner_tcp;
}

struct metadata {
    bit<24> vxlan_vni;
    bit<32> nexthop;
    bit<32> vtepIP;

    // current data
    bit<32>     ip_identification;
    bit<32>     ip_flags;
    bit<32>     ip_fragOffset;
    bit<32>     tcp_seqNo;

    // last packet data
    bit<32>     last_ip_identification;
    bit<32>     last_ip_flags;
    bit<32>     last_ip_fragOffset;
    bit<32>     last_tcp_seqNo;

    // Flags for C++ assemble
    bit<1>      flags_ip;
    bit<1>      flags_tcp;
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser ParserImpl(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            IPV4_ETHTYPE: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            UDP_PROTO: parse_udp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract(hdr.udp);
        transition select(hdr.udp.destPort) {
            UDP_PORT_VXLAN: parse_vxlan;
            default: accept;
        }
    }
    state parse_vxlan {
        packet.extract(hdr.vxlan);
        transition parse_inner_ethernet;
    }
    state parse_inner_ethernet {
        packet.extract(hdr.inner_ethernet);
        transition select(hdr.ethernet.etherType) {
            IPV4_ETHTYPE: parse_inner_ipv4;
            default: accept;
        }
    }
    state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        meta.ip_identification = (bit<32>)hdr.inner_ipv4.identification;
        meta.ip_flags = (bit<32>)hdr.inner_ipv4.flags;
        meta.ip_fragOffset = (bit<32>)hdr.inner_ipv4.fragOffset;

        transition select(hdr.inner_ipv4.protocol) {
            TCP_PROTO: parse_inner_tcp;
            default: accept;
        }
    }
    state parse_inner_tcp {
        packet.extract(hdr.inner_tcp);

        transition accept;
    }
}

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

/*************************************************************************
********************  C O N T R O L  D E S I G N   ***********************
*************************************************************************/
control vxlan_ingress_upstream(inout headers hdr, 
                            inout metadata meta, 
                            inout standard_metadata_t standard_metadata) {

    action vxlan_decap() {
        // as simple as set outer headers as invalid
        hdr.ethernet.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.udp.setInvalid();
        hdr.vxlan.setInvalid();
    }

    table t_vxlan_term {
        key = {
            // Inner Ethernet desintation MAC address of target VM
            hdr.inner_ethernet.dstAddr : exact;
        }

        actions = {
            @defaultonly NoAction;
            vxlan_decap();
        }

    }

    action forward(bit<9> port) {
        standard_metadata.egress_spec = port;
    }

    table t_forward_l2 {
        key = {
            hdr.inner_ethernet.dstAddr : exact;
        }
        actions = {
            forward;
        }
    }

    apply {
        if (hdr.ipv4.isValid()) {
            if (t_vxlan_term.apply().hit) {
                t_forward_l2.apply();
            }
        }
    }
}

control vxlan_ingress_downstream(inout headers hdr, 
                                inout metadata meta, 
                                inout standard_metadata_t standard_metadata) {

    action set_vni(bit<24> vni) {
        meta.vxlan_vni = vni;
    }

    action set_ipv4_nexthop(bit<32> nexthop) {
        meta.nexthop = nexthop;
    }

    table t_vxlan_segment {

        key = {
            hdr.ipv4.dstAddr : lpm;
        }

        actions = {
            @defaultonly NoAction;
            set_vni;
        }

    }

    table t_vxlan_nexthop {

        key = {
            hdr.ethernet.dstAddr : exact;
        }

        actions = {
            set_ipv4_nexthop;
        }
    }

    action set_vtep_ip(bit<32> vtep_ip) {
        meta.vtepIP = vtep_ip;
    }

    table t_vtep {
        key = {
            hdr.ethernet.srcAddr : exact;
        }

        actions = {
            set_vtep_ip;
        }

    }

    action route(bit<9> port) {
        standard_metadata.egress_spec = port;
    }

    table t_vxlan_routing {

        key = {
            meta.nexthop : exact;
        }

        actions = {
            route;
        }
    }

    apply {
        if (hdr.ipv4.isValid()) {
            t_vtep.apply();
            if(t_vxlan_segment.apply().hit) {
                if(t_vxlan_nexthop.apply().hit) {
                    t_vxlan_routing.apply();
                }
            }
        }
    }
}

control vxlan_egress_upstream(inout headers hdr, 
                            inout metadata meta, 
                            inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control vxlan_egress_downstream(inout headers hdr, 
                            inout metadata meta, 
                            inout standard_metadata_t standard_metadata) {

    action rewrite_macs(bit<48> smac, bit<48> dmac) {
        hdr.ethernet.srcAddr = smac;
        hdr.ethernet.dstAddr = dmac;
    }

    table t_send_frame {

            key = {
                hdr.ipv4.dstAddr : exact;
            }

            actions = {
                rewrite_macs;
            }
        }

    action vxlan_encap() {

        hdr.inner_ethernet = hdr.ethernet;
        hdr.inner_ipv4 = hdr.ipv4;

        hdr.ethernet.setValid();

        hdr.ipv4.setValid();
        hdr.ipv4.version = IP_VERSION_4;
        hdr.ipv4.ihl = IPV4_MIN_IHL;
        hdr.ipv4.diffserv = 0;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen
                            + (ETH_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + VXLAN_HDR_SIZE);
        hdr.ipv4.identification = 0x1513; /* From NGIC */
        hdr.ipv4.flags = 0;
        hdr.ipv4.fragOffset = 0;
        hdr.ipv4.ttl = 64;
        hdr.ipv4.protocol = UDP_PROTO;
        hdr.ipv4.dstAddr = meta.nexthop;
        hdr.ipv4.srcAddr = meta.vtepIP;
        hdr.ipv4.hdrChecksum = 0;

        hdr.udp.setValid();
        // The VTEP calculates the source port by performing the hash of the inner Ethernet frame's header.
        hash(hdr.udp.srcPort, HashAlgorithm.crc16, (bit<13>)0, { hdr.inner_ethernet }, (bit<32>)65536);
        hdr.udp.destPort = UDP_PORT_VXLAN;
        hdr.udp.length = hdr.ipv4.totalLen + (UDP_HDR_SIZE + VXLAN_HDR_SIZE);
        hdr.udp.checksum = 0;

        hdr.vxlan.setValid();
        hdr.vxlan.reserved = 0;
        hdr.vxlan.reserved_2 = 0;
        hdr.vxlan.flags = 0;
        hdr.vxlan.vni = meta.vxlan_vni;

    }

    apply {
        if (meta.vxlan_vni != 0) {
            vxlan_encap();
            if (hdr.vxlan.isValid()) {
                t_send_frame.apply();
            }
        }
    }

}

control segment_ingress_assemble(inout headers hdr, 
                            inout metadata meta, 
                            inout standard_metadata_t standard_metadata) {

    apply {

        reg_segment.read(meta.last_ip_identification, 0);
        reg_segment.read(meta.last_ip_flags, 1);
        reg_segment.read(meta.last_ip_fragOffset, 2);
        reg_segment.read(meta.last_tcp_seqNo, 3);

        // IP 切片
        // [Reserved | Don't Fragment (DF) | More Fragments (MF)]
        bit<3> DF = ((bit<3>)meta.last_ip_flags >> 1) & 3w1;
        bit<3> MF = (bit<3>)meta.last_ip_flags & 3w1;
        if (DF == 1 || MF == 0) {
            meta.flags_ip = 0; // 不需要和之前的IP切片拼接
        }
        if (MF == 1) {
            meta.flags_ip = 1; // 当前包需要和之前的IP切片拼接
        }

        // TCP segment assemble
        if (meta.last_tcp_seqNo != hdr.inner_tcp.seqNo) {
            meta.flags_tcp = 1; // 当前包需要和之前的TCP分段拼接
        }
        else {
            meta.flags_tcp = 0; // 不需要和之前的TCP分段拼接
        }

        // 计算下个TCP包期望的seqNo
        bit<16> tcp_total_length = hdr.ipv4.totalLen - 16w20;
        bit<16> tcp_data_length = tcp_total_length - (bit<16>)hdr.inner_tcp.dataOffset * 4;
        bit<32> tcp_next_seqNo = hdr.inner_tcp.seqNo + (bit<32>)tcp_data_length;
        
        // write the result into reg 最后写入寄存器
        reg_segment.write(0, meta.ip_identification);
        reg_segment.write(1, meta.ip_flags);
        reg_segment.write(2, meta.ip_fragOffset);
        reg_segment.write(3, tcp_next_seqNo);
    }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control vxlan_ingress(inout headers hdr, 
                    inout metadata meta, 
                    inout standard_metadata_t standard_metadata) {

    vxlan_ingress_downstream()  downstream;
    vxlan_ingress_upstream()    upstream;

    apply {
        if (hdr.vxlan.isValid()) {
            upstream.apply(hdr, meta, standard_metadata);
        } else {
            downstream.apply(hdr, meta, standard_metadata);
        }

        segment_ingress_assemble.apply(hdr, meta, standard_metadata);
    }
}


/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control vxlan_egress(inout headers hdr, 
                    inout metadata meta, 
                    inout standard_metadata_t standard_metadata) {

    vxlan_egress_downstream()  downstream;

    apply {
        if (!hdr.vxlan.isValid()) {
            downstream.apply(hdr, meta, standard_metadata);
        }
    }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers  hdr, inout metadata meta) {
     apply {
    }
}

/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.vxlan);
        packet.emit(hdr.inner_ethernet);
        packet.emit(hdr.inner_ipv4);
        packet.emit(hdr.inner_tcp);
        }
}
/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
ParserImpl(),
MyVerifyChecksum(),
vxlan_ingress(),
vxlan_egress(),
MyComputeChecksum(),
DeparserImpl()
) main;