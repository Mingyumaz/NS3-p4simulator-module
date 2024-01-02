/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) YEAR COPYRIGHTHOLDER
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: PengKuang <kphf1995cm@outlook.com>
 * Modified: Ma Mingyu <myma979@gmail.com>
 */

#ifndef GLOBAL_H
#define GLOBAL_H
#include "ns3/object.h"
#include "ns3/p4-controller.h"
#include <cstring>
#include <map>
#include <sys/time.h>

namespace ns3 {

#define LOCAL_CALL 0
#define RUNTIME_CLI 1
#define NS3PIFOTM 2
#define NS3 1
#define P4Simulator 0

// nf info
unsigned const int SIMPLESWITCH = 0;
unsigned const int PRIORITYQUEUE = 1;
unsigned const int SIMPLECODEL = 2;
unsigned const int CODELPP = 3;

// match type
unsigned const int EXACT = 0;
unsigned const int LPM = 1;
unsigned const int TERNARY = 2;
unsigned const int VALID = 3;
unsigned const int RANGE = 4;

// get current time (ms)
unsigned long getTickCount(void);

class P4Controller;
class P4GlobalVar : public Object {
public:
  static TypeId GetTypeId(void);
  // controller
  static P4Controller g_p4Controller;

  // switch configuration info
  static unsigned int g_networkFunc;
  static std::string g_flowTablePath; // The file path of the flow table
  static std::string
      g_viewFlowTablePath;         // The file path of the view flow table
  static std::string g_p4JsonPath; // The file path of the p4 json file
  static unsigned int
      g_populateFlowTableWay; // The way to populate the flow table
  /**
   * @brief the bmv2 is not integrated into ns-3 fully, so the control
   * of the bottleneck needs to be set in bmv2 (by setting the packet
   * processing speed of the switch).
   */
  static int g_switchBottleNeck;

  // configure file path info
  static std::string g_homePath;
  static std::string g_ns3RootName;
  static std::string g_ns3SrcName;
  static unsigned int g_nsType;
  static std::string g_nfDir;
  static std::string g_topoDir;
  static std::string g_flowTableDir;
  static std::string g_exampleP4SrcDir;


  // ns-3 and p4 connect name
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

  // tracing info
  static bool ns3_inner_p4_tracing;
  static bool ns3_p4_tracing_dalay_sim;
  static bool ns3_p4_tracing_dalay_ByteTag; // Byte Tag
  static bool ns3_p4_tracing_control;       // how the switch control the pkts
  static bool ns3_p4_tracing_drop;          // the pkts drop in and out switch

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
#endif /*GLOBAL_H*/
