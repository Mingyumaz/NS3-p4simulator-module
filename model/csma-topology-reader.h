/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c)
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
 * Author:
 */

#ifndef CSMA_TOPOLOGY_READER_H
#define CSMA_TOPOLOGY_READER_H

#include "ns3/nstime.h"
#include "p4-topology-reader.h"

namespace ns3 {

// ------------------------------------------------------------
// --------------------------------------------
/**
 * \ingroup topology
 *
 * \brief Topology file reader (Inet-format type).
 *
 * This class takes an input file in Inet format and extracts all
 * the informations needed to build the topology
 * (i.e.number of nodes, links and links structure).
 * It have been tested with Inet 3.0
 * http://topology.eecs.umich.edu/inet/
 *
 * It might set a link attribute named "Weight", corresponding to
 * the euclidean distance between two nodes, the nodes being randomly
 * positioned.
 */
class CsmaTopologyReader : public P4TopologyReader {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId(void);

  CsmaTopologyReader();
  virtual ~CsmaTopologyReader();

  /**
   * \brief Main topology reading function.
   *
   * This method opens an input stream and reads the Inet-format file.
   * From the first line it takes the total number of nodes and links.
   * Then discards a number of rows equals to total nodes (containing
   * useless geographical informations).
   * Then reads until the end of the file (total links number rows) and saves
   * the structure of every single link in the topology.
   *
   * \return The container of the nodes created (or empty container if there was
   * an error)
   */
  virtual void Read(void);

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse.
   */
  CsmaTopologyReader(const CsmaTopologyReader &);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse.
   * \returns
   */
  CsmaTopologyReader &operator=(const CsmaTopologyReader &);

  // end class InetTopologyReader
};

// end namespace ns3
}; // namespace ns3

#endif /* CSMA_TOPOLOGY_READER_H */
