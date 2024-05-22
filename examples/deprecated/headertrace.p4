// example3: l3 forwarding 
/*
 * https://osinstom.github.io/en/tutorial/vxlan-tunneling-in-p4/# 
 *
*/

/* Definitions */

#define ETHERTYPE_BF_FABRIC    0x9000
#define ETHERTYPE_VLAN         0x8100
#define ETHERTYPE_QINQ         0x9100
#define ETHERTYPE_MPLS         0x8847
#define ETHERTYPE_IPV4         0x0800
#define ETHERTYPE_IPV6         0x86dd
#define ETHERTYPE_ARP          0x0806
#define ETHERTYPE_RARP         0x8035
#define ETHERTYPE_NSH          0x894f
#define ETHERTYPE_ETHERNET     0x6558
#define ETHERTYPE_ROCE         0x8915
#define ETHERTYPE_FCOE         0x8906
#define ETHERTYPE_TRILL        0x22f3
#define ETHERTYPE_VNTAG        0x8926
#define ETHERTYPE_LLDP         0x88cc
#define ETHERTYPE_LACP         0x8809

#define IP_PROTOCOLS_ICMP              1
#define IP_PROTOCOLS_IGMP              2
#define IP_PROTOCOLS_IPV4              4
#define IP_PROTOCOLS_TCP               6
#define IP_PROTOCOLS_UDP               17
#define IP_PROTOCOLS_IPV6              41
#define IP_PROTOCOLS_GRE               47
#define IP_PROTOCOLS_IPSEC_ESP         50
#define IP_PROTOCOLS_IPSEC_AH          51
#define IP_PROTOCOLS_ICMPV6            58
#define IP_PROTOCOLS_EIGRP             88
#define IP_PROTOCOLS_OSPF              89
#define IP_PROTOCOLS_PIM               103
#define IP_PROTOCOLS_VRRP              112

#define IP_PROTOCOLS_IPHL_ICMP         0x501
#define IP_PROTOCOLS_IPHL_IPV4         0x504
#define IP_PROTOCOLS_IPHL_TCP          0x506
#define IP_PROTOCOLS_IPHL_UDP          0x511
#define IP_PROTOCOLS_IPHL_IPV6         0x529
#define IP_PROTOCOLS_IPHL_GRE 		   0x52f

#define UDP_PORT_BOOTPS                67
#define UDP_PORT_BOOTPC                68
#define UDP_PORT_RIP                   520
#define UDP_PORT_RIPNG                 521
#define UDP_PORT_DHCPV6_CLIENT         546
#define UDP_PORT_DHCPV6_SERVER         547
#define UDP_PORT_HSRP                  1985
#define UDP_PORT_BFD                   3785
#define UDP_PORT_LISP                  4341
#define UDP_PORT_VXLAN                 4789
#define UDP_PORT_VXLAN_GPE             4790
#define UDP_PORT_ROCE_V2               4791
#define UDP_PORT_GENV                  6081
#define UDP_PORT_SFLOW 				   6343

/* header */

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

// header_type icmp_t {
//     fields {
//         typeCode : 16;
//         hdrChecksum : 16;
//     }
// }

header_type vxlan_t {
    fields {
        flags : 8;
        reserved : 24;
        vni : 24;
        reserved_2 : 24;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        length_ : 16;
        checksum : 16;
    }
}

/* metadata 定义的字段会被编译器自动添加到所有的表中，以便你可以在任何阶段读取或修改它们。
    用户可以再pipeline中修改的数据 */
header_type meta_t {
    fields {
        /* [ [Ip header] [TCP header] [data] ]*/

        // ===========================ipv4 分片===========================
        /*Total Length includes the length of IPv4 header and the 
        Data it carries.
        [存疑]包的总长度，是否是所有切片的总长度和？*/
        totalLen : 16;
        /* When a Datagram is fragmented in to multiple Datagrams, 
        IPv4 give all the fragments the same identification number 
        and this number is used to identify IPv4 fragments at the 
        receiving side.
        唯一标志Number*/
        identification : 16; 
        /* 3 bit -> [Reserved | Don't Fragment (DF) | More Fragments (MF)]
         如果MF为1，则后面还有Fragments，如果为0，则后面无Fragments*/
        ip_flags : 3;
        /* indicates where in the actual IPv4 Datagram this fragment 
        belongs. The fragment offset is measured in units of 8 
        octets (64 bits). The first fragment has offset zero.
        象征着目前该包在整段中的位置*/
        fragOffset : 13;

        // =========================== tcp 分段===========================
        /*https://stackoverflow.com/questions/2259458/how-to-reassemble-tcp-segment 
        https://en.wikipedia.org/wiki/Transmission_Control_Protocol#Packet_structure 
        [CWR ECE URG ACK +PSH RST SYN FIN]
        PSH 标志，如果为1，push buffer，0继续追加到buffer中*/
        tcp_flags : 8;

        // last state temp
        last_totalLen : 16;
        last_identification : 16; 
        last_ip_flags : 3;
        last_fragOffset : 13;
        last_tcp_flags : 8;

        // Flags for C++ assemble 向C++提供的组装切片，段的指令, 1为需要组装，0为不需要组装
        flags_ip : 1;
        flags_tcp : 1;
    }
}

metadata meta_t meta;

/* register */

register reg_last_state {
    // save the meta data
    width: 16;
    instance_count: 8;
}

/* parser */

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;
parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        default: ingress;
    }
}

header ipv4_t ipv4;
parser parse_ipv4 {
    extract(ipv4);
    meta.totalLen = ipv4.totalLen;// 将totalLen字段的值保存到metadata中
    meta.identification = ipv4.identification;
    meta.ip_flags = ipv4.flags;
    meta.fragOffset = ipv4.fragOffset;

    return select(latest.fragOffset, latest.ihl, latest.protocol) {
        // IP_PROTOCOLS_IPHL_ICMP : parse_icmp;
        // IP_PROTOCOLS_IPHL_TCP : parse_tcp;
        IP_PROTOCOLS_IPHL_UDP : parse_udp;
        default: ingress;
    }
}

// header icmp_t icmp;
// parser parse_icmp {
//     extract(icmp);
//     return select(latest.typeCode) { 
//         default: ingress;
//     }
// } 

header udp_t udp;
parser parse_udp {
    extract(udp);
    return select(latest.dstPort) {
        UDP_PORT_VXLAN : parse_vxlan;
        default: ingress;
    }
}

header vxlan_t vxlan;
parser parse_vxlan {
    extract(vxlan);
    return select() {
        default: parse_inner_ethernet;
    }
}

header ethernet_t inner_ethernet;
parser parse_inner_ethernet {
    extract(inner_ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_inner_ipv4;
        default: ingress;
    }
}

header ipv4_t inner_ipv4;
parser parse_inner_ipv4 {
    extract(inner_ipv4);
    return select() {
        IP_PROTOCOLS_IPHL_TCP : parse_inner_tcp;
        default: ingress;
    }
}

header tcp_t inner_tcp;
parser parse_inner_tcp {
    extract(inner_tcp);
    meta.tcp_flags = inner_tcp.flags;

    return select(latest.dstPort) { 
        default: ingress;
    }
}

/* actions */

action _drop() {
	drop();
}

action _nop() {
}

action forward(port) {
	modify_field(standard_metadata.egress_spec, port);
}

action set_register() {
    // Read register -> last state
    register_read(meta.last_totalLen, reg_last_state, 0);
    register_read(meta.last_identification, reg_last_state, 1);
    register_read(meta.last_ip_flags, reg_last_state, 2);
    register_read(meta.last_fragOffset, reg_last_state, 3);
    register_read(meta.last_tcp_flags, reg_last_state, 4);
    
    // save current info into register
    register_write(reg_last_state, 0, meta.totalLen);
    register_write(reg_last_state, 1, meta.identification);
    register_write(reg_last_state, 2, meta.ip_flags);
    register_write(reg_last_state, 3, meta.fragOffset);
    register_write(reg_last_state, 4, meta.tcp_flags);
}

action set_flag() {
}

/* tables */

table l3_forward {
	reads { 
		ipv4.dstAddr : lpm;
	}
	actions {
		_nop; forward;
	}	
}
  
table assemble {
    reads {
        // 目前未设定触发条件，所有包都尝试assemble
    }
    actions {
        set_register;
    }
}

/* control flows */

control ingress {
	apply(l3_forward);
    apply(assemble);

}

control egress {
}

/* deparser */
// control deparser {
//     apply()
//         packet.emit(hdr.ethernet);
//         packet.emit(hdr.ipv4);
//         packet.emit(hdr.udp);
//         packet.emit(hdr.vxlan);
//         packet.emit(hdr.inner_ethernet);
//         packet.emit(hdr.inner_ipv4);
//         packet.emit(hdr.inner_tcp);
//     }
// }