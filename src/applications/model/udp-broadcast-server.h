/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#ifndef UDP_BROADCAST_SERVER_H
#define UDP_BROADCAST_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include <random>

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications 
 * \defgroup udpBroadcast UdpBroadcast
 */

/**
 * \ingroup udpBroadcast
 * \brief A Udp Broadcast server
 *
 * Every packet received is sent back.
 */
class UdpBroadcastServer : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  UdpBroadcastServer ();
  virtual ~UdpBroadcastServer ();

  /**
   * \brief Saves the address of a peer
   * \param address Address of the peer where to send the packets
   */
  void AddPeerAddress (Address address);

  /**
   * \brief Sets the size of the forwarder packet
   * \param pkt_size Size of the packet
   */
  void SetPacketSize (int pkt_size);

  /**
   * \brief Sets the size of the forwarder packet
   * \param mean Mean of the normal distribution
   */
  void SetMean (float mean);

  /**
   * \brief Sets the size of the forwarder packet
   * \param std_dev Standard deviation of the normal distribution
   */
  void SetStandardDeviation (float std_dev);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);

  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<Socket> m_socket; //!< IPv4 Socket
  Ptr<Socket> m_socket6; //!< IPv6 Socket
  Address m_local; //!< local multicast address
  std::vector<Address> m_peerAddresses; //!< Peer addresses
  int m_pktSize; //!< Size of the forwarded packet
  float m_mean = 0; //!< Mean of the normal distribution
  float m_stdDev = 0; //!< Standard deviation of the normal distribution
  std::default_random_engine generator; //!< Random number generator
  std::normal_distribution<float> distribution; //!< Normal distribution

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_BROADCAST_SERVER_H */

