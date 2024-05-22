#ifndef GLOBAL_H
#define GLOBAL_H

#include "ns3/object.h"
#include "ns3/p4-controller.h"
#include <cstring>
#include <map>
#include <sys/time.h>

namespace ns3 {

// Method to configure the flow table in P4 switch
#define LOCAL_CALL 0  // Local call to configure the flow table
#define RUNTIME_CLI 1 // Use runtime CLI to configure the flow table
#define NS3PIFOTM 2   // Use thrift port to configure the flow table

// Network function type
#define NS3 1         // Normal switch
#define P4Simulator 0 // P4 switch

// P4 example number
const unsigned int ROUTER = 0;
const unsigned int FIREWALL = 1;
const unsigned int SILKROAD = 2;
const unsigned int SIMPLE_ROUTER = 3;
const unsigned int COUNTER = 4;
const unsigned int METER = 5;
const unsigned int REGISTER = 6;
const unsigned int SIMPLESWITCH = 7;
const unsigned int PRIORITYQUEUE = 8;
const unsigned int SIMPLECODEL = 9;
const unsigned int CODELPP = 10;

// Match type in the table of the P4 switch
const unsigned int EXACT = 0;
const unsigned int LPM = 1;
const unsigned int TERNARY = 2;
const unsigned int VALID = 3;
const unsigned int RANGE = 4;

// Get current time (ms)
unsigned long getTickCount(void);

class P4Controller;

class P4GlobalVar : public Object {
public:
  static TypeId GetTypeId(void);

  // Controller
  static P4Controller g_p4Controller;

  // Switch configuration info
  static unsigned int g_networkFunc;
  static std::string g_p4MatchTypePath;
  static std::string g_flowTablePath;
  static std::string g_viewFlowTablePath;
  static std::string g_p4JsonPath;
  static unsigned int g_populateFlowTableWay;

  /**
   * @brief For the scheduling algorithm, the BMv2 switch is using
   * non-work-conserving method, it sets the time for packet departure, which
   * may pend the packet and leave the link idle to shape the traffic [Related
   * paper: BMW Tree: Large-scale, High-throughput and Modular PIFO
   * Implementation using Balanced Multi-Way Sorting Tree]. There are actually
   * many scheduling algorithms but this simulator does not implement them.
   *
   * The BMv2 is not integrated into ns-3 fully, so the control
   * of the bottleneck needs to be set in BMv2 (by setting the packet scheduling
   * speed of the switch).
   */
  static int g_switchBottleNeck;

  // Configure file path info
  static std::string g_homePath;
  static std::string g_ns3RootName;
  static std::string g_ns3SrcName;
  static unsigned int g_nsType;
  static std::string g_nfDir;
  static std::string g_topoDir;
  static std::string g_flowTableDir;
  static std::string g_exampleP4SrcDir;

  // ns-3 and P4 connect name
  static std::string ns3i_drop_1;
  static std::string ns3i_drop_2;
  static std::string ns3i_priority_id_1;
  static std::string ns3i_priority_id_2;
  static std::string ns3i_protocol_1;
  static std::string ns3i_protocol_2;
  static std::string ns3i_destination_1;
  static std::string ns3i_destination_2;
  static std::string ns3i_pkts_id_1;
  static std::string ns3i_pkts_id_2;

  // Tracing info
  static bool ns3_inner_p4_tracing;
  static bool ns3_p4_tracing_dalay_sim;
  static bool ns3_p4_tracing_dalay_ByteTag; // Byte Tag
  static bool ns3_p4_tracing_control; // How the switch controls the packets
  static bool ns3_p4_tracing_drop;    // Packets drop in and out of the switch

  static std::map<std::string, unsigned int> g_nfStrUintMap;
  static void SetP4MatchTypeJsonPath();
  static void InitNfStrUintMap();

private:
  P4GlobalVar();
  ~P4GlobalVar();
  P4GlobalVar(const P4GlobalVar &);
  P4GlobalVar &operator=(const P4GlobalVar &);
};

} // namespace ns3

#endif /* GLOBAL_H */
