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

#ifndef UDP_MIX_SERVER_H
#define UDP_MIX_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \brief Wrapper class for the Packet class.
 * 
 * It stores and retrieves information to and from the given packet
 */
class IoMusTPacket : public Object
{
  public:

    IoMusTPacket ();
    IoMusTPacket (Ptr<Packet> packet);
    IoMusTPacket (Ptr<IoMusTPacket> packet);
    virtual ~IoMusTPacket ();

    /**
    * \brief Returns the send time contained in the packet
    */
    virtual Time get_send_time(void);

    /**
    * \brief Returns the sequence number contained in the packet
    */
    virtual int get_seq_n(void);

    /**
     * \brief Sets the sequence number
     * 
     * \param seq_n The sequence number
     */
    virtual void set_seq_n(int seq_n);

    /**
    * \brief Returns the original packet
    */
    virtual Ptr<Packet> get_packet(void);

    /**
    * \brief Retrieves the address of the destination where the packet is ment to be sent
    */
    virtual Address get_to_address(void);

    /**
    * \brief Copies the data containing the payload of the packet
    * to a new memory location stored in the provided pointer
    */
    virtual void copy_data(uint8_t *data);

    /**
    * \brief Retrieves the size of the payload of the packet
    */
    virtual uint32_t get_pkt_size(void);

  protected:
    virtual void DoDelete (void);

  private:
    Time sendTime; //!< The time when the packet has been sent by the client
    int seqN; //!< The sequence number of the packet
    Address to; //!< The address contained in the packet
    uint8_t *data; //!< Pointer to the payload of the packet
    uint32_t pktSize; //!< Size of the payload of the packet
};

/**
 * \brief An audio stream between a client and a mixing server
 * 
 */
class Stream : public Object
{
  public:
    Stream (Address client_address);
    virtual ~Stream ();

    /**
     * \brief Add a packet to the send buffer
     * 
     * \param packet The packet to be added
     * \returns The normalized sequence number of the packet
     */
    virtual int add_packet(Ptr<IoMusTPacket> packet);

    /**
     * \brief Retrieve the packet at the given normalized sequence number
     * 
     * \param seqN The normalized sequence number
     * \returns A pointer to the corresponding packet
     */
    virtual Ptr<IoMusTPacket> get_packet(int seqN);

    /**
     * \brief Marks the packet at the specified position as sent
     * 
     * \param seqN The normalized sequence number
     */
    virtual void set_packet_sent(int seqN);

    /**
    * \brief Returns the address of the client associated with this stream
    * 
    * \returns The address of the client associated with this stream
    */
    virtual Address get_address(void);

    /**
    * \brief Sets the offset
    * 
    * \param offset Number to be applied to the sequence number to get the normalized
    * sequence number
    */
    virtual void set_offset(int offset);

    /**
    * \brief Returns the offset to be applied to the sequence number to get the
    * normalized sequence number
    * 
    * \returns The offset to be applied to the sequence number to get the
    * normalized sequence number
    */
    virtual int get_offset(void);

    /**
     * \brief Converts the given normalized sequence number to the correspondent sequence
     * number of this stream
     * 
     * \param norm_seq_n The normalized sequence number
     * 
     * \returns The sequence number of this stream
     */
    virtual int convert_seq_n(int norm_seq_n);

  protected:
    virtual void DoDelete (void);

  private:
    static constexpr int array_size = 100; //!< The size of the array containing the send buffer
    Address address; //!< The client's ipv4 address
    std::array<Ptr<IoMusTPacket>, array_size> send_buffer; //!< Array of packets waiting to be mixed
    int offset; //!< Number to be applied to the sequence number to get the normalized sequence number
};

/**
 * \ingroup applications 
 * \defgroup udpmix UdpMix
 */

/**
 * \ingroup udpmix
 * \brief A Udp Mix server
 *
 * This class emulates the mix server of a Newtorked
 * Music Performance system. It receives packets from
 * all clients and takes care of reordering them and
 * synchronizing them as if they had to be mixed.
 * The "mixed" packets are then sent out to each
 * client when all packets of a temporal slot are
 * received or after a pre-determined timout.
 */
class UdpMixServer : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  UdpMixServer ();
  virtual ~UdpMixServer ();

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
  virtual void send_packets(int seq_n);

  /**
  * \brief Checks if the packets with the specified normalized
  * sequence number have already been sent out, otherwise it
  * sends them to all the currently available clients.
  * 
  * \param seq_n The normalized sequence number
  */
  virtual void check_and_send_packets(int seq_n);

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
  virtual void add_packet(Ptr<IoMusTPacket> packet, Address from, Ptr<Stream> stream);

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
  bool first_packet = true; //!< Flag that identifies the very first packet
  std::vector<Ptr<Stream>> streams; //!< The collection of currently active streams
  Time reference_time = MicroSeconds(0); //!< Send time of the first packet of the first stream
  Time packet_transmission_period; //!< Time between subsequent packets at the sender side.
  Time timeout; //!< Time to wait for missing packets before sending out the ones that arrived.
  Ptr<Socket> socket; //!< Pointer to the socket used to send out packets.
  int oldest_seq_n = 0; //!< The lowest normalized sequence number that packets must have to be accepted

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_Mix_SERVER_H */

