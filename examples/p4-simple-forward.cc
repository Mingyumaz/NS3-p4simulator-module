/*
Command: 
./waf --run "p4-simple-forward -model=0 -sim_delay=true -trace_control=true -trace_drop=False -pcap=true"
./waf --run p4-simple-forward --command-template="gdb %s --args --p4src=codel++"

Topo design like:
      h0--------------|                    |---------------h5 (UDP or TCP)
                      |                    |                        priority 7
--------------------------------------------------------------------------------
                      |                    |
      h1--------------s0------------------s1---------------h4 (UDP or TCP)
                      |                    |                        priority 3
--------------------------------------------------------------------------------
                      |                    |
      h2--------------|                    |---------------h3  UDP
                                                                    priority 0

*/

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/p4-helper.h"
#include "ns3/v4ping-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/binary-tree-topo-helper.h"
#include "ns3/fattree-topo-helper.h"
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fstream> 
#include "ns3/global.h"
#include "ns3/p4-topology-reader-helper.h"
#include "ns3/helper.h"
#include "ns3/build-flowtable-helper.h"
#include "ns3/traffic-control-module.h"
// Include the necessary header file
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ScratchP4Forward");

// total packets receive/ total packets send
uint32_t totalPacketsReceivedH3 = 0; // n2 ---> n3
uint32_t totalPacketsReceivedH4 = 0; // n1 ---> n4
uint32_t totalPacketsReceivedH5 = 0; // n0 ---> n5 
uint32_t totalPacketsSendH2 = 0;
uint32_t totalPacketsSendH1 = 0;
uint32_t totalPacketsSendH0 = 0;

static void TagTx(Ptr<const ns3::Packet> p)
{
    DelayJitterEstimation delayJitter;
    delayJitter.PrepareTx(p);
}

void CalculateDelay(Ptr<const ns3::Packet> p, const Address& address, std::string filename)
{
    DelayJitterEstimation delayJitter;
    delayJitter.RecordRx(p);
    Time t = delayJitter.GetLastDelay();

    // Record delay in .csv file
    std::ofstream delayFile(filename, std::ios::app);
    if (delayFile.is_open()) {
        delayFile << Simulator::Now().GetSeconds() << "," << t.GetMilliSeconds() << std::endl;
        delayFile.close();
    } else {
        std::cout << "file " << filename << " can not open!" << std::endl;
    }
}

void CalculateDelay1(Ptr<const ns3::Packet> p, const Address& address)
{
    CalculateDelay(p, address, "./scratch-data/p4-codel/sim_delay_n0n5.csv");
}

void CalculateDelay2(Ptr<const ns3::Packet> p, const Address& address)
{
    CalculateDelay(p, address, "./scratch-data/p4-codel/sim_delay_n1n4.csv");
}

void CalculateDelay3(Ptr<const ns3::Packet> p, const Address& address)
{
    CalculateDelay(p, address, "./scratch-data/p4-codel/sim_delay_n2n3.csv");
}

void IncrementSendH0(Ptr<const ns3::Packet> p){
    totalPacketsSendH0++;
}

void IncrementSendH1(Ptr<const ns3::Packet> p){
    totalPacketsSendH1++;
}

void IncrementSendH2(Ptr<const ns3::Packet> p){
    totalPacketsSendH2++;
}

void IncrementReceivedH5(Ptr<const ns3::Packet> p, const Address &address){
    totalPacketsReceivedH5++;
}

void IncrementReceivedH4(Ptr<const ns3::Packet> p, const Address &address){
    totalPacketsReceivedH4++;
}

void IncrementReceivedH3(Ptr<const ns3::Packet> p, const Address &address){
    totalPacketsReceivedH3++;
}

// ============================ data struct ============================
struct SwitchNodeC_t {
    NetDeviceContainer switchDevices;
    std::vector<std::string> switchPortInfos;
};

struct HostNodeC_t {
    NetDeviceContainer hostDevice;
    Ipv4InterfaceContainer hostIpv4;
    unsigned int linkSwitchIndex;
    unsigned int linkSwitchPort;
    std::string hostIpv4Str;
};

int main(int argc, char* argv[])
{

    // ============================  init global variable for P4 ============================
    P4GlobalVar::g_homePath = "/home/p4/";
    P4GlobalVar::g_ns3RootName = "";
    P4GlobalVar::g_ns3SrcName = "ns-3-dev-git/";
    P4GlobalVar::g_nfDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName + P4GlobalVar::g_ns3SrcName + "src/p4simulator/examples/p4src/";
    P4GlobalVar::g_topoDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName + P4GlobalVar::g_ns3SrcName + "src/p4simulator/examples/topo/";
    P4GlobalVar::g_populateFlowTableWay = NS3PIFOTM; // the method to send the config for flow table, LOCAL_CALL/RUNTIME_CLI/NS3PIFOTM
    P4GlobalVar::g_nsType = P4Simulator; // NS3 / P4Simulator
    // P4GlobalVar::g_runtimeCliTime = 3; // the time sleep before send the config command(flow table) through the thrift to bmv2
    SwitchApi::InitApiMap();
    P4GlobalVar::InitNfStrUintMap();

    // Do tracing or NOT.
    P4GlobalVar::ns3_p4_tracing_dalay_sim = true; // Byte Tag tracing simulation time
    P4GlobalVar::ns3_p4_tracing_control = true; // how the switch control the pkts
    P4GlobalVar::ns3_p4_tracing_drop = false; // the pkts drop in and out switch

    // ============================  init global variable for simulation =====================
    int podNum = 2; // the host number, default 2
    bool buildTopo = false; // whether build topo by podNum
    bool toBuild = true; // whether build flow table entired by program --(should always be true with p4)
    uint16_t pktSize = 1470;  //in bytes. 1458 to prevent fragments, should always < MTU
    
    std::string appDataRate[] = {"2Mbps", "2Mbps", "2Mbps"}; 
    std::string p4src = "simple_switch";
    bool enableTracePcap = true;
    
    uint32_t SentPackets = 0;
	uint32_t ReceivedPackets = 0;
	uint32_t LostPackets = 0;
	bool TraceMetric = false;

    const std::string outputDir = "./scratch-data/";
    const std::string fileName = "simple_switch";

    const size_t depth_pkts_all = 10000; // all egress queue depths maxinum
    
    // Here we need calculated the congestion, how many packets we want to pass the queue
    uint64_t congestion_bottleneck = 5; // Mbps
    uint64_t rate_pps = (uint64_t)(congestion_bottleneck * 1024 * 1024 / (pktSize * 8));
    P4GlobalVar::g_switchBottleNeck = (uint64_t)(1000000 / rate_pps); // pps/us

    // The times
    double global_start_time = 1.0;
    double sink_start_time = global_start_time + 1.0;
    double client_start_time = global_start_time + 2.0;
    
    double client_stop_time = client_start_time + 10; // 10s simulation
    double global_stop_time = client_stop_time + 10;
    double sink_stop_time = client_stop_time + 10;

    // ============================ start debug module ============================
    LogComponentEnable("ScratchP4Forward", LOG_LEVEL_LOGIC);
    
    //  ============================ import the topo ============================
    std::string topoFormat("CsmaTopo");
    std::string topoPath = P4GlobalVar::g_topoDir;
    if (buildTopo) {
        // topoPath += "testTopo.txt";
        topoPath += "dumbbellTopo.txt";
    } else {
        topoPath += "dumbbellTopo.txt";
    }
    std::string topoInput(topoPath);

    // ============================  command line ============================
    CommandLine cmd;
    cmd.AddValue("model", "Select P4Simulator[0] or NS3[1]", P4GlobalVar::g_nsType);
    cmd.AddValue("podnum", "Numbers of built tree topo levels", podNum); // build the tree topo automate
    cmd.AddValue("build", "Build flow table entries by program[true] or not[false]", toBuild);
    cmd.AddValue("sim_delay", "Trace simulation delay by program[true] or not[false]", P4GlobalVar::ns3_p4_tracing_dalay_sim);
    cmd.AddValue("trace_control", "Trace packet control by p4[true] or not[false]", P4GlobalVar::ns3_p4_tracing_control);
    cmd.AddValue("trace_drop", "Trace packet drop by p4[true] or not[false]", P4GlobalVar::ns3_p4_tracing_drop);
    cmd.AddValue("p4src", "the algorithm of the p4-switch, [codel+], [codel++], [codel++v2], [codel_recir], [new_codel],[new_codel_v2], [simple_switch], [simple_codel], [priority_queuing]", p4src);
    cmd.AddValue("pcap", "Trace packet pacp [true] or not[false]", enableTracePcap);
    cmd.Parse(argc, argv);

    // ============================ ns-3 <----> bmv2 ============================

    // the p4 simulator(ns-3) connect with BMv2 pipeline (get tracing value, get pkts_ID etc.)
    //      if the p4 change the headers and strcut, for example "scalars.userMetadata._ns3i_ns3_drop18"
    //      the "18" maybe change to other number, this will cause the ERROR. 
    if (p4src == "simple_switch" or p4src == "priority_queuing") {
        P4GlobalVar::ns3i_drop_1 = "scalars.userMetadata._ns3i_ns3_drop1";
        P4GlobalVar::ns3i_drop_2 = "scalars.userMetadata._ns3i_ns3_drop1";
        P4GlobalVar::ns3i_priority_id_1 = "scalars.userMetadata._ns3i_ns3_priority_id2";
        P4GlobalVar::ns3i_priority_id_2 = "scalars.userMetadata._ns3i_ns3_priority_id2";
        P4GlobalVar::ns3i_protocol_1 = "scalars.userMetadata._ns3i_protocol3";
        P4GlobalVar::ns3i_protocol_2 = "scalars.userMetadata._ns3i_protocol3";
        P4GlobalVar::ns3i_destination_1 = "scalars.userMetadata._ns3i_destination4";
        P4GlobalVar::ns3i_destination_2 = "scalars.userMetadata._ns3i_destination4";
        P4GlobalVar::ns3i_pkts_id_1 = "scalars.userMetadata._ns3i_pkts_id5";
        P4GlobalVar::ns3i_pkts_id_2 = "scalars.userMetadata._ns3i_pkts_id5";
    }
    else {
        std::cout << "Now can only using simple_switch as P4 switch!" << std::endl;
    }

    // ============================ build topo automate ============================
    if (buildTopo) {
        // FattreeTopoHelper treeTopo(podNum, topoPath);
        BinaryTreeTopoHelper treeTopo(podNum, topoPath);
        treeTopo.Write(); // This will override the configure file of topo using the <topoFormat> value.
    }

    // ============================ topo -> network ============================
    // loading from topo file --> gene topo(linking the nodes)
    P4TopologyReaderHelper p4TopoHelp;
    p4TopoHelp.SetFileName(topoInput);
    p4TopoHelp.SetFileType(topoFormat);
    Ptr<P4TopologyReader> topoReader = p4TopoHelp.GetTopologyReader();
    if (topoReader != 0) {
        topoReader->Read();
    }
    if (topoReader->LinksSize() == 0) {
        NS_LOG_ERROR("Problems reading the topology file. Failing.");
        return -1;
    }

    // get switch and host node
    NodeContainer hosts = topoReader->GetHostNodeContainer();
    NodeContainer csmaSwitch = topoReader->GetSwitchNodeContainer();
    const unsigned int hostNum = hosts.GetN();
    const unsigned int switchNum = csmaSwitch.GetN();

    // get switch network function
    std::vector<std::string> switchNetFunc = topoReader->GetSwitchNetFunc();

    // NS_LOG_LOGIC("======= switchNum:" << switchNum << "      " << "hostNum:" << hostNum << " =======");

    // set default network link parameter
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps")); //@todo
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(0.01)));

    // init network link info
    P4TopologyReader::ConstLinksIterator_t iter;
    SwitchNodeC_t switchNodes[switchNum];
    HostNodeC_t hostNodes[hostNum];
    unsigned int fromIndex, toIndex;
    std::string dataRate, delay;
    for (iter = topoReader->LinksBegin(); iter != topoReader->LinksEnd(); iter++) {
        if (iter->GetAttributeFailSafe("DataRate", dataRate))
            csma.SetChannelAttribute("DataRate", StringValue(dataRate));
        if (iter->GetAttributeFailSafe("Delay", delay))
            csma.SetChannelAttribute("Delay", StringValue(delay));

        NetDeviceContainer link = csma.Install(NodeContainer(iter->GetFromNode(), iter->GetToNode()));
        fromIndex = iter->GetFromIndex();
        toIndex = iter->GetToIndex();
        if (iter->GetFromType() == 's' && iter->GetToType() == 's') {
            unsigned int fromSwitchPortNumber = switchNodes[fromIndex].switchDevices.GetN();
            unsigned int toSwitchPortNumber = switchNodes[toIndex].switchDevices.GetN();
            switchNodes[fromIndex].switchDevices.Add(link.Get(0));
            switchNodes[fromIndex].switchPortInfos.push_back("s" + UintToString(toIndex) + "_" + UintToString(toSwitchPortNumber));

            switchNodes[toIndex].switchDevices.Add(link.Get(1));
            switchNodes[toIndex].switchPortInfos.push_back("s" + UintToString(fromIndex) + "_" + UintToString(fromSwitchPortNumber));
        } else {
            if (iter->GetFromType() == 's' && iter->GetToType() == 'h') {
                unsigned int fromSwitchPortNumber = switchNodes[fromIndex].switchDevices.GetN();
                switchNodes[fromIndex].switchDevices.Add(link.Get(0));
                switchNodes[fromIndex].switchPortInfos.push_back("h" + UintToString(toIndex - switchNum));

                hostNodes[toIndex - switchNum].hostDevice.Add(link.Get(1));
                hostNodes[toIndex - switchNum].linkSwitchIndex = fromIndex;
                hostNodes[toIndex - switchNum].linkSwitchPort = fromSwitchPortNumber;
            } else {
                if (iter->GetFromType() == 'h' && iter->GetToType() == 's') {
                    unsigned int toSwitchPortNumber = switchNodes[toIndex].switchDevices.GetN();
                    switchNodes[toIndex].switchDevices.Add(link.Get(1));
                    switchNodes[toIndex].switchPortInfos.push_back("h" + UintToString(fromIndex - switchNum));

                    hostNodes[fromIndex - switchNum].hostDevice.Add(link.Get(0));
                    hostNodes[fromIndex - switchNum].linkSwitchIndex = toIndex;
                    hostNodes[fromIndex - switchNum].linkSwitchPort = toSwitchPortNumber;
                } else {
                    // NS_LOG_LOGIC("link error!");
                    abort();
                }
            }
        }
    }

    // ============================ print topo info ============================
    // view host link info
    // for (unsigned int i = 0; i < hostNum; i++)
    //     std::cout << "host " << i << ": connect with switch " << hostNodes[i].linkSwitchIndex << " port " << hostNodes[i].linkSwitchPort << std::endl;

    // // view switch port info
    // for (unsigned int i = 0; i < switchNum; i++) {
    //     std::cout << "switch " << i << " connect with: ";
    //     for (size_t k = 0; k < switchNodes[i].switchPortInfos.size(); k++)
    //         std::cout << switchNodes[i].switchPortInfos[k] << " ";
    //     std::cout << std::endl;
    // }

    // ============================ add internet stack to the hosts ============================
    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(csmaSwitch); // required by "traffic control" in switch

    // ============================ traffic control ===========================

    /*TrafficControlHelper tch;
    tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc"); // simple traffic control, FIFO, no other process
    // only backbone link has traffic control
    QueueDiscContainer queuediscs[switchNum];
    for (unsigned int i = 0; i < switchNum; i++) {
        queuediscs[i] = tch.Install (switchNodes[i].switchDevices);
    }*/

    // ============================ assign ip address ============================

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.0.0", "255.255.0.0");
    for (unsigned int i = 0; i < hostNum; i++) {
        hostNodes[i].hostIpv4 = ipv4.Assign(hostNodes[i].hostDevice);
        hostNodes[i].hostIpv4Str = Uint32ipToHex(hostNodes[i].hostIpv4.GetAddress(0).Get());
        // print the ip address
        //     Ipv4Address temp = hostNodes[i].hostIpv4.GetAddress(0);
        //     std::cout << "The ip of host " << i << " is: ";
        //     temp.Print(std::cout);
        //     std::cout << std::endl;
    }

    // build needed parameter to build flow table entries
    std::vector<unsigned int> linkSwitchIndex(hostNum);
    std::vector<unsigned int> linkSwitchPort(hostNum);
    std::vector<std::string> hostIpv4(hostNum);
    std::vector<std::vector<std::string>> switchPortInfo(switchNum);
    for (unsigned int i = 0; i < hostNum; i++) {
        linkSwitchIndex[i] = hostNodes[i].linkSwitchIndex;
        linkSwitchPort[i] = hostNodes[i].linkSwitchPort;
        hostIpv4[i] = hostNodes[i].hostIpv4Str;
    }
    for (unsigned int i = 0; i < switchNum; i++) {
        switchPortInfo[i] = switchNodes[i].switchPortInfos;
    }

    // ***************************setting for the MAC address********************
    // @mingyu
    NetDeviceContainer host_devices;
    NetDeviceContainer switch_devices;

    for (unsigned int i = 0; i < hostNum; i++) {
        Ptr<Node> nodehost = hosts.Get(i);
        Ptr<NetDevice> device = nodehost->GetDevice(0);
        host_devices.Add(device);
    }

    Mac48Address address("00:00:0a:00:01:01");
    Ptr<NetDevice> device = host_devices.Get(0);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:00:01:02");
    device = host_devices.Get(1);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:00:01:03");
    device = host_devices.Get(2);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:00:01:04");
    device = host_devices.Get(3);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:00:01:05");
    device = host_devices.Get(4);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:00:01:06");
    device = host_devices.Get(5);
    device->SetAddress(address);

    for (unsigned int i = 0; i < switchNum; i++) {
        for (int j = 0; j < 4; j++) {
            Ptr<Node> nodeswitch = csmaSwitch.Get(i);
            Ptr<NetDevice> device = nodeswitch->GetDevice(j);
            switch_devices.Add(device);
        }
    }
    address = Mac48Address("00:00:0a:01:00:01");
    device = switch_devices.Get(0);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:01:00:02");
    device = switch_devices.Get(1);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:01:00:03");
    device = switch_devices.Get(2);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:01:00:04");
    device = switch_devices.Get(3);
    device->SetAddress(address);

    address = Mac48Address("00:00:0a:02:00:01");
    device = switch_devices.Get(4);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:02:00:02");
    device = switch_devices.Get(5);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:02:00:03");
    device = switch_devices.Get(6);
    device->SetAddress(address);
    address = Mac48Address("00:00:0a:02:00:04");
    device = switch_devices.Get(7);
    device->SetAddress(address);

    // ***************************** print node id *****************************

    // // NS_LOG_LOGIC("***************************** print node id *****************************");
    // for (unsigned int i = 0; i < hostNum; i++) {
    //     Ptr<Node> nodehost = hosts.Get(i);
    //     // std::cout << "host " << i << " is with id: " << nodehost->GetId() << std::endl;
    //     // cout the mac address
    //     Ptr<NetDevice> device = nodehost->GetDevice(0);
    //     if (device == 0) {
    //         NS_LOG_LOGIC ("Could not get NetDevice with index ");
    //     }
    //     else{
    //         Mac48Address address;
    //         address.ConvertFrom(device->GetAddress ());
    //         NS_LOG_LOGIC ("Device in host " << i << " MAC address: " << address);
    //     }
    // }
    // // for (unsigned int i = 0; i < switchNum; i++) {
    // //     Ptr<Node> nodeswitch = csmaSwitch.Get(i);
    // //     std::cout << "switch " << i << " is with id: " << nodeswitch->GetId() << std::endl;
    // // }
    // // NS_LOG_LOGIC("*****************************      *****************************");

    // ============================ flow table ============================
    // build flow table entries by program
    if (toBuild && P4GlobalVar::g_nsType == P4Simulator) {
        // NS_LOG_LOGIC("BuildFlowtableHelper");
        // BuildFlowtableHelper flowtableHelper("fattree",podNum);
        BuildFlowtableHelper flowtableHelper;
        flowtableHelper.Build(linkSwitchIndex, linkSwitchPort, hostIpv4, switchPortInfo);
        // flowtableHelper.Write(P4GlobalVar::g_flowTableDir); // p4 with version-16 should deal it by handwork
        //  std::cout << "=== Show the flow table of switch ===" << std::endl;
        //  flowtableHelper.Show();
        //  std::cout << "===-------------------------------===" << std::endl;
    }

    // bridge switch and switch devices
    if (P4GlobalVar::g_nsType == P4Simulator) {
        std::string flowTableName;
        P4Helper bridge;
        for (unsigned int i = 0; i < switchNum; i++) {
            flowTableName = "CLI" + UintToString(i + 1);
            P4GlobalVar::g_networkFunc = P4GlobalVar::g_nfStrUintMap[switchNetFunc[i]];
            P4GlobalVar::SetP4MatchTypeJsonPath();

            // Here we set the Json Path by hand (different switchs with different configurations)
            // @json_path [codel+], [codel++], [codel++v2], [codel_recir], [new_codel] or [new_codel_v2]
            // [simple_switch], [simple_codel], [priority_queuing]
            if (p4src == "simple_switch") {
                P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_exampleP4SrcDir + "simple_switch/simple_switch.json";
                P4GlobalVar::g_flowTableDir = P4GlobalVar::g_exampleP4SrcDir + "simple_switch/flowtable/";  
            } else {
                // No need for other configuration. The config from the topo file.
                // std::cout << "Using TOPO file defined alg for P4" << std::endl; 
                NS_LOG_LOGIC("Using TOPO file defined alg for P4");
            }
            P4GlobalVar::g_flowTablePath = P4GlobalVar::g_flowTableDir + flowTableName;

            NS_LOG_LOGIC("Configuring with json: " << P4GlobalVar::g_p4JsonPath);
            NS_LOG_LOGIC("Configuring with CLI: " << P4GlobalVar::g_flowTablePath);
            bridge.Install(csmaSwitch.Get(i), switchNodes[i].switchDevices);

            int num_devices = csmaSwitch.Get(i)->GetNDevices();
            Ptr<NetDevice> net_device = csmaSwitch.Get(i)->GetDevice(num_devices - 1); // should be the last device

            if (!net_device) {
                std::cout << "Not found BridgeNetDevice" << std::endl;
            } else {
                // Here we schedule the receive pkts from the other Threads
                Ptr<P4NetDevice> p4_net_device = DynamicCast<P4NetDevice>(net_device);
                P4Model* p4_model = p4_net_device->GetP4Model();

                p4_model->set_all_egress_queue_depths(depth_pkts_all);
                p4_model->set_all_egress_queue_rates(rate_pps); // allocate all resources.
            }
        }
    } else {
        BridgeHelper bridge;
        for (unsigned int i = 0; i < switchNum; i++) {
            bridge.Install(csmaSwitch.Get(i), switchNodes[i].switchDevices);
        }
    }

    // ============================ application ============================
    //
    Config::SetDefault("ns3::Ipv4RawSocketImpl::Protocol", StringValue("2"));
    std::vector<OnOffHelper> onOffs; // saving all the onOff applications(using to change the attribute)

    // == First == send link n0 -----> n5

    unsigned int serverI = 5; // with hostNum 0
    uint16_t servPort = 9093; // setting for port
    Ipv4Address serverAddr1 = hostNodes[serverI].hostIpv4.GetAddress(0);
    InetSocketAddress dst1 = InetSocketAddress(serverAddr1, servPort);
    PacketSinkHelper sink1 = PacketSinkHelper("ns3::UdpSocketFactory", dst1);
    ApplicationContainer sinkApp1 = sink1.Install(hosts.Get(serverI));

    sinkApp1.Start(Seconds(sink_start_time));
    sinkApp1.Stop(Seconds(sink_stop_time));

    OnOffHelper onOff1("ns3::UdpSocketFactory", dst1);
    onOff1.SetAttribute("PacketSize", UintegerValue(pktSize));
    onOff1.SetAttribute("DataRate", StringValue(appDataRate[0]));
    onOff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]")); // always send the packages
    onOff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]")); // never stop
    onOffs.push_back(onOff1);

    ApplicationContainer app1 = onOff1.Install(hosts.Get(0));
    app1.Start(Seconds(client_start_time));
    app1.Stop(Seconds(client_stop_time));

    // == Second == send link n1 -----> n4

    serverI = 4; // with hostNum 1
    Ipv4Address serverAddr2 = hostNodes[serverI].hostIpv4.GetAddress(0);
    InetSocketAddress dst2 = InetSocketAddress(serverAddr2, servPort);
    PacketSinkHelper sink2 = PacketSinkHelper("ns3::UdpSocketFactory", dst2);
    ApplicationContainer sinkApp2 = sink2.Install(hosts.Get(serverI));

    sinkApp2.Start(Seconds(sink_start_time));
    sinkApp2.Stop(Seconds(sink_stop_time));

    OnOffHelper onOff2("ns3::UdpSocketFactory", dst2);
    onOff2.SetAttribute("PacketSize", UintegerValue(pktSize));
    onOff2.SetAttribute("DataRate", StringValue(appDataRate[1]));
    onOff2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]")); // always send the packages
    onOff2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]")); // never stop
    onOffs.push_back(onOff2);

    ApplicationContainer app2 = onOff2.Install(hosts.Get(1));
    app2.Start(Seconds(client_start_time));
    app2.Stop(Seconds(client_stop_time));

    // == Third == send link n2 -----> n3

    serverI = 3; // with hostNum 2
    Ipv4Address serverAddr3 = hostNodes[serverI].hostIpv4.GetAddress(0);
    InetSocketAddress dst3 = InetSocketAddress(serverAddr3, servPort);
    PacketSinkHelper sink3 = PacketSinkHelper("ns3::UdpSocketFactory", dst3);
    ApplicationContainer sinkApp3 = sink3.Install(hosts.Get(serverI));

    sinkApp3.Start(Seconds(sink_start_time));
    sinkApp3.Stop(Seconds(sink_stop_time));

    OnOffHelper onOff3("ns3::UdpSocketFactory", dst3);
    onOff3.SetAttribute("PacketSize", UintegerValue(pktSize));
    onOff3.SetAttribute("DataRate", StringValue(appDataRate[2]));
    onOff3.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]")); // always send the packages
    onOff3.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]")); // never stop
    onOffs.push_back(onOff3);

    ApplicationContainer app3 = onOff3.Install(hosts.Get(2));
    app3.Start(Seconds(client_start_time));
    app3.Stop(Seconds(client_stop_time));
    

    // ============================== tracing ==============================

    Ptr<OnOffApplication> ptr_app1 = DynamicCast<OnOffApplication>(hosts.Get(0)->GetApplication(0));
    Ptr<OnOffApplication> ptr_app2 = DynamicCast<OnOffApplication>(hosts.Get(1)->GetApplication(0));
    Ptr<OnOffApplication> ptr_app3 = DynamicCast<OnOffApplication>(hosts.Get(2)->GetApplication(0));
    // ============================== delay tracing ==============================
   
    if (P4GlobalVar::ns3_p4_tracing_dalay_sim) {
        // trace the link delay from n0 ---> n5 by Tag
        ptr_app1->TraceConnectWithoutContext("Tx", MakeCallback(&TagTx));
        sinkApp1.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&CalculateDelay1));

        // trace the link delay from n1 ---> n4 by Tag
        ptr_app2->TraceConnectWithoutContext("Tx", MakeCallback(&TagTx));
        sinkApp2.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&CalculateDelay2));

        // trace the link delay from n2 ---> n3 by Tag
        ptr_app3->TraceConnectWithoutContext("Tx", MakeCallback(&TagTx));
        sinkApp3.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&CalculateDelay3));
    }

    // ============================== packet loss ==============================

    ptr_app1->TraceConnectWithoutContext("Tx", MakeCallback(&IncrementSendH0));
    sinkApp1.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&IncrementReceivedH5));

    ptr_app2->TraceConnectWithoutContext("Tx", MakeCallback(&IncrementSendH1));
    sinkApp2.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&IncrementReceivedH4));

    ptr_app3->TraceConnectWithoutContext("Tx", MakeCallback(&IncrementSendH2));
    sinkApp3.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&IncrementReceivedH3));

    // ============================== pcap ==============================
    if (enableTracePcap) {
        csma.EnablePcapAll("ScratchP4Codel", enableTracePcap);
        ns3::Packet::EnablePrinting();
    }

    // ============================== flow monitor ==============================
    // Create a flow monitor helper
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // ============================== simulation ==============================
    Simulator::Stop(Seconds(global_stop_time));
    Simulator::Run();
    Simulator::Destroy();

    //flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);
    if (TraceMetric)
	{
		int j=0;
		float AvgThroughput = 0;
		Time Jitter;
		Time Delay;

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
		std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
		{
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

        NS_LOG_UNCOND("----Flow ID:" <<iter->first);
        NS_LOG_UNCOND("Src Addr" <<t.sourceAddress << "Dst Addr "<< t.destinationAddress);
        NS_LOG_UNCOND("Sent Packets=" <<iter->second.txPackets);
        NS_LOG_UNCOND("Received Packets =" <<iter->second.rxPackets);
        NS_LOG_UNCOND("Lost Packets =" <<iter->second.txPackets-iter->second.rxPackets);
        NS_LOG_UNCOND("Packet delivery ratio =" <<iter->second.rxPackets*100/iter->second.txPackets << "%");
        NS_LOG_UNCOND("Packet loss ratio =" << (iter->second.txPackets-iter->second.rxPackets)*100/iter->second.txPackets << "%");
        NS_LOG_UNCOND("Delay =" <<iter->second.delaySum);
        NS_LOG_UNCOND("Jitter =" <<iter->second.jitterSum);
        NS_LOG_UNCOND("Throughput =" <<iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024<<"Kbps");

        SentPackets = SentPackets +(iter->second.txPackets);
        ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
        LostPackets = LostPackets + (iter->second.txPackets-iter->second.rxPackets);
        AvgThroughput = AvgThroughput + (iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024);
        Delay = Delay + (iter->second.delaySum);
        Jitter = Jitter + (iter->second.jitterSum);

        j = j + 1;

		}

		AvgThroughput = AvgThroughput/j;
		NS_LOG_UNCOND("--------Total Results of the simulation----------"<<std::endl);
		NS_LOG_UNCOND("Total sent packets  =" << SentPackets);
		NS_LOG_UNCOND("Total Received Packets =" << ReceivedPackets);
		NS_LOG_UNCOND("Total Lost Packets =" << LostPackets);
		NS_LOG_UNCOND("Packet Loss ratio =" << ((LostPackets*100)/SentPackets)<< "%");
		NS_LOG_UNCOND("Packet delivery ratio =" << ((ReceivedPackets*100)/SentPackets)<< "%");
		NS_LOG_UNCOND("Average Throughput =" << AvgThroughput<< "Kbps");
		NS_LOG_UNCOND("End to End Delay =" << Delay);
		NS_LOG_UNCOND("End to End Jitter delay =" << Jitter);
		NS_LOG_UNCOND("Total Flod id " << j);
		monitor->SerializeToXmlFile("manet-routing.xml", true, true);
	}

    double drop_n0_n5 = (double)(totalPacketsSendH0 - totalPacketsReceivedH5) / totalPacketsSendH0 * 100;
    double drop_n1_n4 = (double)(totalPacketsSendH1 - totalPacketsReceivedH4) / totalPacketsSendH1 * 100;
    double drop_n2_n3 = (double)(totalPacketsSendH2 - totalPacketsReceivedH3) / totalPacketsSendH2 * 100;

    NS_LOG_LOGIC("total Packets send H0: " << totalPacketsSendH0  << "\n" <<
                "total Packets received H5: " << totalPacketsReceivedH5 << "\n" <<
                "total Packets send H1: " << totalPacketsSendH1  << "\n" <<
                "total Packets received H4: " << totalPacketsReceivedH4 << "\n" <<
                "total Packets send H2: " << totalPacketsSendH2  << "\n" <<
                "total Packets received H3: " << totalPacketsReceivedH3 << "\n" <<
                "Pakcet loss 7 in %: " << drop_n0_n5 << "%" << "\n" <<
                "Pakcet loss 3 in %: " << drop_n1_n4 << "%" << "\n" <<
                "Pakcet loss 0 in %: " << drop_n2_n3 << "%" << "\n");
    return 0;
}