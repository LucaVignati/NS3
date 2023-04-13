/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef UDP_MIX_MEC_SERVER_H
#define UDP_MIX_MEC_SERVER_H
#define OFFSET 10000

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/udp-mix-server.h"

namespace ns3 {

class Socket;
class Packet;

class UdpMixMecServer : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  UdpMixMecServer ();
  virtual ~UdpMixMecServer ();

  /**
   * \brief Saves the address of the client
   * \param address Address of the client where to send the packets
   */
  void SetClientAddress (Address address);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
  * \brief Sends all the given packets with the specified normalized
  * sequence number to all the currently available clients.
  * 
  * \param seq_n The normalized sequence number
  */
  virtual void send_packets(uint32_t seq_n);

  /**
  * \brief Checks if the packets with the specified normalized
  * sequence number have already been sent out, otherwise it
  * sends them to all the currently available clients.
  * 
  * \param seq_n The normalized sequence number
  */
  virtual void check_and_send_packets(uint32_t seq_n);

  /**
  * \brief Instanciates a stream and initializes it.
  * 
  * \param client_address The address of the client associated with the stream.
  * \param packet The first packet from the new stream.
  * 
  * \returns A pointer to the newly created stream.
  */
  virtual Ptr<Stream> add_stream(Address client_address, Ptr<IoMusTPacket> packet);

  /**
  * \brief Add the latest packet to the send buffer of the corresponding stream.
  * 
  * \param packet The latest packet.
  * \param from The address of the client that sent the packet.
  * \param stream The stream associated to the sender.
  */
  virtual void add_packet(Ptr<IoMusTPacket> packet, Ptr<Stream> stream);

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
  Address m_clientAddress; //!< Address of the client where to send the packets
  bool firstPacket = true; //!< Flag that identifies the very first packet
  std::vector<Ptr<Stream>> streams; //!< The collection of currently active streams
  Time reference_time = MicroSeconds(0); //!< Send time of the first packet of the first stream
  Time packetTransmissionPeriod; //!< Time between subsequent packets at the sender side.
  Time timeout; //!< Time to wait for missing packets before sending out the ones that arrived.
  Ptr<Socket> socket; //!< Pointer to the socket used to send out packets.
  uint32_t oldestSeqN = 0; //!< The lowest normalized sequence number that packets must have to be accepted
  uint32_t newestSeqN = 0;//!< The higest normalized sequence number received thus far

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_Mix_MEC_SERVER_H */

