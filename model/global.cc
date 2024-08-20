#include "ns3/global.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("P4GlobalVar");
NS_OBJECT_ENSURE_REGISTERED(P4GlobalVar);

P4Controller P4GlobalVar::g_p4Controller;

/*******************************
 *    P4 switch configuration  *
 *            info             *
 *******************************/

unsigned int P4GlobalVar::g_networkFunc = SIMPLESWITCH;
std::string P4GlobalVar::g_flowTablePath = "";
std::string P4GlobalVar::g_viewFlowTablePath = "";
std::string P4GlobalVar::g_p4MatchTypePath = "";
unsigned int P4GlobalVar::g_populateFlowTableWay =
    RUNTIME_CLI; // LOCAL_CALL/RUNTIME_CLI
std::string P4GlobalVar::g_p4JsonPath = "";
int P4GlobalVar::g_switchBottleNeck = 10000;

std::string P4GlobalVar::g_homePath = "/home/p4/";
std::string P4GlobalVar::g_ns3RootName = "/";
std::string P4GlobalVar::g_ns3SrcName = "ns-3-dev-git/";

std::string P4GlobalVar::g_nfDir =
    P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
    P4GlobalVar::g_ns3SrcName + "src/p4simulator/examples/p4src/";
std::string P4GlobalVar::g_topoDir =
    P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
    P4GlobalVar::g_ns3SrcName + "src/p4simulator/examples/topo/";
std::string P4GlobalVar::g_flowTableDir =
    P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
    P4GlobalVar::g_ns3SrcName + "scratch-p4-file/flowtable/";
std::string P4GlobalVar::g_exampleP4SrcDir =
    P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
    P4GlobalVar::g_ns3SrcName + "src/p4simulator/examples/p4src/";

unsigned int P4GlobalVar::g_nsType = P4Simulator;
std::map<std::string, unsigned int> P4GlobalVar::g_nfStrUintMap;

/*******************************
 *    P4 switch tracing data   *
 *******************************/
std::string P4GlobalVar::ns3i_drop_1 = "scalars.userMetadata._ns3i_ns3_drop18";
std::string P4GlobalVar::ns3i_drop_2 = "scalars.userMetadata._ns3i_ns3_drop14";
std::string P4GlobalVar::ns3i_priority_id_1 =
    "scalars.userMetadata._ns3i_ns3_priority_id19";
std::string P4GlobalVar::ns3i_priority_id_2 =
    "scalars.userMetadata._ns3i_ns3_priority_id15";
std::string P4GlobalVar::ns3i_protocol_1 =
    "scalars.userMetadata._ns3i_protocol20";
std::string P4GlobalVar::ns3i_protocol_2 =
    "scalars.userMetadata._ns3i_protocol16";
std::string P4GlobalVar::ns3i_destination_1 =
    "scalars.userMetadata._ns3i_destination21";
std::string P4GlobalVar::ns3i_destination_2 =
    "scalars.userMetadata._ns3i_destination17";
std::string P4GlobalVar::ns3i_pkts_id_1 =
    "scalars.userMetadata._ns3i_pkts_id22";
std::string P4GlobalVar::ns3i_pkts_id_2 =
    "scalars.userMetadata._ns3i_pkts_id18";
bool P4GlobalVar::ns3_inner_p4_tracing = false;
bool P4GlobalVar::ns3_p4_tracing_dalay_sim = false;
bool P4GlobalVar::ns3_p4_tracing_dalay_ByteTag = false; // Byte Tag
bool P4GlobalVar::ns3_p4_tracing_control =
    false; // how the switch control the pkts
bool P4GlobalVar::ns3_p4_tracing_drop =
    false; // the pkts drop in and out switch

// get the current time in milliseconds
unsigned long getTickCount(void) {
  unsigned long currentTime = 0;

#ifdef WIN32
  // Windows Platform
  currentTime = GetTickCount();
#else
  // Unix/Linux Platform
  struct timeval current;
  gettimeofday(&current, NULL);
  currentTime = current.tv_sec * 1000 + current.tv_usec / 1000;
#endif

#ifdef OS_VXWORKS
  // VXWorks Platform
  ULONGA timeSecond = tickGet() / sysClkRateGet();
  ULONGA timeMilsec = tickGet() % sysClkRateGet() * 1000 / sysClkRateGet();
  currentTime = timeSecond * 1000 + timeMilsec;
#endif

  return currentTime;
}

void P4GlobalVar::SetP4MatchTypeJsonPath() {
  switch (P4GlobalVar::g_networkFunc) {
  // simple switch for new p4-model
  case FIREWALL: {
    P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "firewall/firewall.json";
    P4GlobalVar::g_p4MatchTypePath =
        P4GlobalVar::g_nfDir + "firewall/mtype.txt";
    break;
  }
  case SILKROAD: {
    P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "silkroad/silkroad.json";
    P4GlobalVar::g_p4MatchTypePath =
        P4GlobalVar::g_nfDir + "silkroad/mtype.txt";
    break;
  }
  case ROUTER: {
    P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "router/router.json";
    P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "router/mtype.txt";
    break;
  }
  case SIMPLE_ROUTER: {
    P4GlobalVar::g_p4JsonPath =
        P4GlobalVar::g_nfDir + "simple_router/simple_router.json";
    P4GlobalVar::g_p4MatchTypePath =
        P4GlobalVar::g_nfDir + "simple_router/mtype.txt";
    break;
  }
  case COUNTER: {
    P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "counter/counter.json";
    P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "counter/mtype.txt";
    break;
  }
  case METER: {
    P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "meter/meter.json";
    P4GlobalVar::g_p4MatchTypePath = P4GlobalVar::g_nfDir + "meter/mtype.txt";
    break;
  }
  case REGISTER: {
    P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "register/register.json";
    P4GlobalVar::g_p4MatchTypePath =
        P4GlobalVar::g_nfDir + "register/mtype.txt";
    break;
  }
  case SIMPLESWITCH: {
    P4GlobalVar::g_p4JsonPath =
        P4GlobalVar::g_nfDir + "simple_switch/simple_switch.json";
    P4GlobalVar::g_flowTableDir =
        P4GlobalVar::g_nfDir + "simple_switch/flowtable/";
    break;
  }
  case PRIORITYQUEUE: {
    P4GlobalVar::g_p4JsonPath =
        P4GlobalVar::g_nfDir + "priority_queuing/priority_queuing.json";
    P4GlobalVar::g_flowTableDir =
        P4GlobalVar::g_nfDir + "priority_queuing/flowtable/";
    break;
  }
  case SIMPLECODEL: {
    P4GlobalVar::g_p4JsonPath =
        P4GlobalVar::g_nfDir + "simple_codel/simple_codel.json";
    P4GlobalVar::g_flowTableDir =
        P4GlobalVar::g_nfDir + "simple_codel/flowtable/";
    break;
  }
  case CODELPP: {
    P4GlobalVar::g_p4JsonPath = P4GlobalVar::g_nfDir + "codelpp/codel1.json";
    P4GlobalVar::g_flowTableDir = P4GlobalVar::g_nfDir + "codelpp/flowtable/";
    break;
  }
  default: {
    std::cerr << "NETWORK_FUNCTION_NO_EXIST!!!" << std::endl;
    break;
  }
  }
}

void P4GlobalVar::InitNfStrUintMap() {
  P4GlobalVar::g_nfStrUintMap["ROUTER"] = ROUTER;
  P4GlobalVar::g_nfStrUintMap["SIMPLE_ROUTER"] = SIMPLE_ROUTER;
  P4GlobalVar::g_nfStrUintMap["FIREWALL"] = FIREWALL;
  P4GlobalVar::g_nfStrUintMap["SILKROAD"] = SILKROAD;
  P4GlobalVar::g_nfStrUintMap["COUNTER"] = COUNTER;
  P4GlobalVar::g_nfStrUintMap["METER"] = METER;
  P4GlobalVar::g_nfStrUintMap["REGISTER"] = REGISTER;
  P4GlobalVar::g_nfStrUintMap["SIMPLESWITCH"] = SIMPLESWITCH;
  P4GlobalVar::g_nfStrUintMap["PRIORITYQUEUE"] = PRIORITYQUEUE;
  P4GlobalVar::g_nfStrUintMap["SIMPLECODEL"] = SIMPLECODEL;
  P4GlobalVar::g_nfStrUintMap["CODELPP"] = CODELPP;
}

TypeId P4GlobalVar::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::P4GlobalVar")
                          .SetParent<Object>()
                          .SetGroupName("P4GlobalVar");
  return tid;
}
P4GlobalVar::P4GlobalVar() { NS_LOG_FUNCTION(this); }

P4GlobalVar::~P4GlobalVar() { NS_LOG_FUNCTION(this); }
} // namespace ns3
