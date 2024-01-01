/* -*- P4_16 -*- */

/*
* Copyright 2018-present Ralf Kundel, Nikolas Eller
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
* 
* Author: Tobias Scheinert
* ref-src: srcP4/Evaluation/5_Router_TS_CoDel_Prio_ECN/codel_1.p4
* Modified by: Mingyu, used for test
*
* v1model.p4: https://github.com/p4lang/p4c/blob/main/p4include/v1model.p4
* 
* Warning: there is modified to suit for ns3, see @ns-3
*/

#include <core.p4>
#include <v1model.p4>

// CoDel time # using ts_res = std::chrono::microseconds (src in bmv2 code)

#define SOJOURN_TARGET_2 5000 // 5 ms
#define SOJOURN_TARGET_1 SOJOURN_TARGET_2
#define SOJOURN_TARGET_0 SOJOURN_TARGET_2
#define SOJOURN_TARGET_x SOJOURN_TARGET_2

#define CONTROL_INTERVAL_0 48w100000 // 100ms
#define CONTROL_INTERVAL_1 48w100000 // 100ms
#define CONTROL_INTERVAL_2 48w100000 // 100ms
#define CONTROL_INTERVAL_x 1000000 // 100ms  

#define INTERFACE_MTU 1500
#define NUM_QUEUE 32w64
const bit<16> TYPE_IPV4 = 0x800;
const bit<16> TYPE_ARP = 0x806;

// CoDel registers
register<bit<32>>(NUM_QUEUE) r_codel_drop_count;
register<bit<48>>(NUM_QUEUE) r_codel_drop_time;
register<bit<32>>(NUM_QUEUE) r_codel_last_drop_count;
register<bit<48>>(NUM_QUEUE) r_codel_next_drop;
register<bit<1>>(NUM_QUEUE) r_codel_state_dropping;

// RED 
#define MIN_DROP_TCP 8w20
#define MIN_DROP_UDP 8w50
#define MAX_DROP 8w80
#define NO_DROP 8w0

// slicing queue length registers
register<bit<8>>(NUM_QUEUE) r_slice_drop; // queue_length_limit_pre_drop // 时间可以设定为codel的一个轮回时间
register<bit<19>>(NUM_QUEUE) r_slice_threshold_min; // threshold for queue_length_limit_pre_drop
register<bit<19>>(NUM_QUEUE) r_slice_threshold_max; // threshold for queue_length_limit_pre_drop

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

struct codel_t {
    bit<48> drop_time;
    bit<48> time_now;
    bit<1>  ok_to_drop;
    bit<1>  state_dropping;
    bit<32> delta;
    bit<48> time_since_last_dropping;
    bit<48> drop_next;
    bit<32> drop_cnt;
    bit<32> last_drop_cnt;
    bit<1>  reset_drop_time;
    bit<48> new_drop_time;
    bit<48> new_drop_time_helper;
    bit<9>  queue_id;
}

struct prio_t {
	bit<8>  prio_drop;
	bit<32> time;
	bit<19> enq;
	bit<19> deq;
}

struct routing_metadata_t {
    bit<32> nhop_ipv4;
}

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
    bit<16> sourcePort;
    bit<16> destPort;
    bit<16> length_;
    bit<16> checksum;
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

header arp_t {
    bit<16> hw_type;
    bit<16> protocol_type;
    bit<8>  hw_size;
    bit<8>  protocol_size;
    bit<16> opcode;
    macAddr_t srcMac;
    ip4Addr_t srcIp;
    macAddr_t dstMac;
    ip4Addr_t dstIp;
}

// info for connect with @ns-3, this will be use by ns-3. Others simulators can remove that.
struct ns3info {
    // Please do not change unless you know what you are doing
    bit<1>      ns3_drop;           // the pkts will drop in bmv2 or not
    bit<64>     ns3_priority_id;    // The pkts ID in this prioirty, used for drop tracing. 
    bit<16>     protocol;           // the protocol in ns3::packet
    bit<16>     destination;        // the destination in ns3::packet
    bit<64>     pkts_id;            // the id of the ns3::packet (using for tracing etc)
}

struct flowinfo {
    bit<1>      is_tcp;             // is_tcp = 0 (UDP), is_tcp = 1 (TCP)
}

struct metadata {
    routing_metadata_t      routing_metadata;
    codel_t                 codel;
    prio_t		            prio;
    ns3info                 ns3i;
    flowinfo                flowi;
}

struct headers {
    arp_t           arp;
    ethernet_t      ethernet;
    ipv4_t          ipv4;
    tcp_t           tcp;
    udp_t           udp;
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4 : parse_ipv4;
            TYPE_ARP  : parse_arp;
            default   : accept;
        }
    }

    state parse_arp {
        packet.extract(hdr.arp);
        transition accept;
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w17: parse_udp;
            8w6: parse_tcp;
            default: accept;
        }
    }

    state parse_tcp {
        packet.extract(hdr.tcp);
        meta.flowi.is_tcp = 1w1;
        transition accept;
    }
    
    state parse_udp {
        packet.extract(hdr.udp);
        meta.flowi.is_tcp = 1w0;
        transition accept;
    }

}

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  
        verify_checksum(
            true, 
            { 
                hdr.ipv4.version, 
                hdr.ipv4.ihl, 
                hdr.ipv4.diffserv, 
                hdr.ipv4.totalLen, 
                hdr.ipv4.identification, 
                hdr.ipv4.flags, 
                hdr.ipv4.fragOffset, 
                hdr.ipv4.ttl, 
                hdr.ipv4.protocol, 
                hdr.ipv4.srcAddr, 
                hdr.ipv4.dstAddr 
            }, 
            hdr.ipv4.hdrChecksum, 
            HashAlgorithm.csum16
        );
    }
}

/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    action drop() {
        meta.ns3i.ns3_drop = 1;      // add for connect ns-3 --> drop @ns3
        meta.ns3i.destination = 0;
        meta.ns3i.protocol = 0;
        meta.ns3i.pkts_id = 0;
        meta.ns3i.ns3_priority_id = 0;
        mark_to_drop(standard_metadata);
    }

    action set_port(bit<9> port) {
        standard_metadata.egress_spec = port;
        standard_metadata.egress_port = port;
    }

    action set_arp_nhop(bit<32> nhop_ipv4) {
        meta.routing_metadata.nhop_ipv4 = nhop_ipv4;
    }

    action set_ipv4_nhop(bit<32> nhop_ipv4) {
        meta.routing_metadata.nhop_ipv4 = nhop_ipv4;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 8w1;
    }

    table arp_nhop {
        actions = {
            set_arp_nhop;
            drop;
        }
        key = {
            hdr.arp.dstIp: exact;
        }
        size = 1024;
    }

    table forward_table {
        actions = {
            set_port;
            drop;
        }
        key = {
            meta.routing_metadata.nhop_ipv4: exact;
        }
        size = 1024;
    }

    table ipv4_nhop {
        actions = {
            set_ipv4_nhop;
            drop;
        }
        key = {
            hdr.ipv4.dstAddr: exact;
        }
        size = 1024;
    }

    action set_priority(bit<7> value) {
        standard_metadata.priority = (bit<3>)value;
        // 赋予CoDel_queue_id
        meta.codel.queue_id = (bit<9>)value;
        //meta.codel.queue_id = standard_metadata.egress_port; // 之前的id=port
    }
    
    table slice_priority{
    	actions = {
            set_priority;
        }
        key = {
            hdr.ipv4.srcAddr: exact;            
        }
        size = 1024;
        const default_action = set_priority((bit<7>) 0);
    }

    apply {
        if (hdr.ipv4.isValid() && hdr.ipv4.ttl > 8w0 || hdr.arp.isValid()) {
            // 
            if (hdr.ipv4.isValid() && hdr.ipv4.ttl > 8w0) {
                ipv4_nhop.apply();
            } else {
                if (hdr.arp.isValid()) {
                    arp_nhop.apply();
                }
            }

            forward_table.apply();
            slice_priority.apply(); // slicing and set codel.queue_id

            if (standard_metadata.priority == 0) {
                // Ingress pre-drop
                r_slice_drop.read(meta.prio.prio_drop, (bit<32>)meta.codel.queue_id);
                bit<8> rand_val;
                random<bit<8>>(rand_val, 0, 99);
                if (meta.prio.prio_drop != 8w0) {
                    if (rand_val < meta.prio.prio_drop) {
                        meta.ns3i.ns3_drop = 1; // connect ns-3 --> drop @ns-3 
                        mark_to_drop(standard_metadata); // queue length --> drop @bmv2
                    }
                }
            }
                
            if (meta.flowi.is_tcp == 1w1) {
            r_slice_drop.write((bit<32>)meta.codel.queue_id, NO_DROP); // drop once and stop
            }
           
        }
    }
}

/*************************************************************************
********************  C O N T R O L  D E S I G N   ***********************
*************************************************************************/

control c_codel(inout headers hdr, 
                inout metadata meta, 
                inout standard_metadata_t standard_metadata) {

    action a_codel_control_law(bit<48> value) {
        meta.codel.drop_next = meta.codel.time_now + value;
        r_codel_next_drop.write((bit<32>)meta.codel.queue_id, (bit<48>)meta.codel.drop_next);
    }

    action a_codel_init(bit<48> control_interval) {
        meta.codel.ok_to_drop = 1w0;
        meta.codel.time_now = (bit<48>)standard_metadata.enq_timestamp + (bit<48>)standard_metadata.deq_timedelta;
        meta.codel.new_drop_time = meta.codel.time_now + control_interval;
        r_codel_state_dropping.read(meta.codel.state_dropping, (bit<32>)meta.codel.queue_id);
        r_codel_drop_count.read(meta.codel.drop_cnt, (bit<32>)meta.codel.queue_id);
        r_codel_last_drop_count.read(meta.codel.last_drop_cnt, (bit<32>)meta.codel.queue_id);
        r_codel_next_drop.read(meta.codel.drop_next, (bit<32>)meta.codel.queue_id);
        r_codel_drop_time.read(meta.codel.drop_time, (bit<32>)meta.codel.queue_id);
    }

    action a_go_to_drop_state() {
        meta.ns3i.ns3_drop = 1;      // add for connect ns-3 --> drop @ns3 
	    mark_to_drop(standard_metadata);
        r_codel_state_dropping.write((bit<32>)meta.codel.queue_id, (bit<1>)1);
        meta.codel.delta = meta.codel.drop_cnt - meta.codel.last_drop_cnt;
        meta.codel.time_since_last_dropping = meta.codel.time_now - meta.codel.drop_next;
        meta.codel.drop_cnt = 32w1;
        r_codel_drop_count.write((bit<32>)meta.codel.queue_id, (bit<32>)1);
    }

    table t_codel_control_law {
        actions = {
            a_codel_control_law;
        }
        key = {
            meta.codel.drop_cnt: lpm;
        }
        size = 32;
    }

    apply {
        // preprocesc CoDel parameters for each slice
        bit<32> sojourn_target = 0;
        bit<48> control_interval = 0;
        if (standard_metadata.priority == 3w7) {
            sojourn_target = SOJOURN_TARGET_0;
            control_interval = CONTROL_INTERVAL_0;
        }
        else if (standard_metadata.priority == 3w3) {
            sojourn_target = SOJOURN_TARGET_1;
            control_interval = CONTROL_INTERVAL_1;
        }
        else if (standard_metadata.priority == 3w0) {
            sojourn_target = SOJOURN_TARGET_2;
            control_interval = CONTROL_INTERVAL_2;
        }
        else {
            sojourn_target = SOJOURN_TARGET_x;
            control_interval = CONTROL_INTERVAL_x;
        }
        // CoDel init
        a_codel_init(control_interval);
    
        // CoDel measurements
        
        if (standard_metadata.deq_timedelta < sojourn_target ) { //|| standard_metadata.deq_qdepth < 19w1
            meta.codel.reset_drop_time = 1w1;   // @todo this will always be true!!!
        }
        if (meta.codel.reset_drop_time == 1w1) {
            // 延迟时间短，对应queue_id的droptime为 0，写入寄存器
            r_codel_drop_time.write((bit<32>)meta.codel.queue_id, (bit<48>)0); // write 写入寄存器，read为从寄存器中读取
            meta.codel.drop_time = 48w0;
            r_slice_threshold_min.write((bit<32>)meta.codel.queue_id, (bit<19>)500); // 延迟低。 No limit for queue
        }
        else {
            // delay > sojourn_target
            
            // minimal threshold for queue_length
            r_slice_threshold_min.write((bit<32>)meta.codel.queue_id, (bit<19>)standard_metadata.enq_qdepth);

            if (meta.codel.drop_time == 48w0) {
                // 当次延迟时间长，然而drop time为0（之前延迟时间不长），不需要drop，设定新的drop time
                r_codel_drop_time.write((bit<32>)meta.codel.queue_id, (bit<48>)meta.codel.new_drop_time);
                meta.codel.drop_time = meta.codel.new_drop_time;
            }
            else { //if (meta.codel.drop_time > 48w0)
                // 前几次延迟时间均长，drop time 不为0，需要drop
                if (meta.codel.time_now >= meta.codel.drop_time) {
                    meta.codel.ok_to_drop = 1w1; // 准备 drop
                }
            }
        }

        // CoDel drop control pipeline
        if (meta.codel.state_dropping == 1w1) {
            // IN DROP STATE
            if (meta.codel.ok_to_drop == 1w0) {
                // leave DROP STATE
                r_codel_state_dropping.write((bit<32>)meta.codel.queue_id, (bit<1>)0); //leave drop state
            }
            else {
                // do drop (drop state & ok to drop)
                if (meta.codel.time_now >= meta.codel.drop_next) {
                    // store the current queue length as a threshold(when CoDel triggered)
                    r_slice_threshold_max.write((bit<32>)meta.codel.queue_id, (bit<19>)standard_metadata.enq_qdepth);

                    meta.ns3i.ns3_drop = 1;      // connect ns-3 --> drop @ns-3 
                    mark_to_drop(standard_metadata);
                    meta.codel.drop_cnt = meta.codel.drop_cnt + 32w1; // CoDel drop count++
                    r_codel_drop_count.write((bit<32>)meta.codel.queue_id, (bit<32>)meta.codel.drop_cnt);
                    t_codel_control_law.apply();
                }

            }

        }
        else {
            // Not in DROP STATE
            if (meta.codel.ok_to_drop == 1w1) {
                if (standard_metadata.priority != 3w7) {
                    // the highest priority will not go into the DROP state.
                    a_go_to_drop_state();

                    if (meta.codel.delta > 32w1 && meta.codel.time_since_last_dropping < control_interval*16) {
                        // fast start for CoDel
                        r_codel_drop_count.write((bit<32>)meta.codel.queue_id, (bit<32>)meta.codel.delta);
                        meta.codel.drop_cnt = meta.codel.delta;
                    }
                    r_codel_last_drop_count.write((bit<32>)meta.codel.queue_id, (bit<32>)meta.codel.drop_cnt);
                    t_codel_control_law.apply();
                }
            }
        }
    }
}


/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    
    c_codel() c_codel_0;

    apply {

        // CoDel apply
        c_codel_0.apply(hdr, meta, standard_metadata);

        if (standard_metadata.priority != 3w7) {
            // queue length pre-dropping with threshold
            bit <19> current_queue_threshold_max = 19w0;
            bit <19> current_queue_threshold_min = 19w0;
            r_slice_threshold_max.read(current_queue_threshold_max, (bit<32>)meta.codel.queue_id);
            r_slice_threshold_min.read(current_queue_threshold_min, (bit<32>)meta.codel.queue_id);
            
            if (current_queue_threshold_min > current_queue_threshold_max){
                // in any case this should not happen
                current_queue_threshold_min = current_queue_threshold_max;
            }

            if (standard_metadata.deq_qdepth > current_queue_threshold_min){
                if (meta.flowi.is_tcp == 1w1) {
                    r_slice_drop.write((bit<32>)meta.codel.queue_id, MIN_DROP_TCP); // drop with threshold(min)
                }
                else {
                    r_slice_drop.write((bit<32>)meta.codel.queue_id, MIN_DROP_UDP); // drop with threshold(min)
                }
                
            }
            else if (standard_metadata.deq_qdepth > current_queue_threshold_max){
                r_slice_drop.write((bit<32>)meta.codel.queue_id, MAX_DROP); // drop with threshold(max)
            }
            else{
                r_slice_drop.write((bit<32>)meta.codel.queue_id, NO_DROP); // no drop
            }
        }

    
	}
}
   

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers  hdr, inout metadata meta) {
     apply {
        update_checksum(
        hdr.ipv4.isValid(),
            { hdr.ipv4.version,
              hdr.ipv4.ihl,
              hdr.ipv4.diffserv,
              hdr.ipv4.totalLen,
              hdr.ipv4.identification,
              hdr.ipv4.flags,
              hdr.ipv4.fragOffset,
              hdr.ipv4.ttl,
              hdr.ipv4.protocol,
              hdr.ipv4.srcAddr,
              hdr.ipv4.dstAddr },
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16);
    }
}

/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.arp);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;