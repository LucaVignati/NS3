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
    IoMusTPacket (Ptr<IoMusTPacket> packet, int seq_n);
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
    * \brief Returns the original packet
    */
    virtual Ptr<Packet> get_packet(void);

    /**
    * \brief Retrieves the address of the sender
    */
    virtual Address get_to_address();

    /**
    * \brief Retrieves the address of the sender
    */
    virtual void copy_data(uint8_t *data);

    /**
    * \brief Retrieves the address of the sender
    */
    virtual uint32_t get_pkt_size(void);

  protected:
    virtual void DoDelete (void);

  private:
    Time sendTime;
    int seqN;
    Address to;
    uint8_t *data;
    uint32_t pktSize;
};

/**
 * \brief An audio stream between a client and a mixing server
 * 
 * Stores the arrival times of the latest packets.
 */
class Stream : public Object
{
  public:
    Stream (Address client_address);
    virtual ~Stream ();

    // /**
    // * \brief Add the arrival time of the last packet that arrived.
    // * 
    // * \param arrival_time the arrival time of the last packet that arrived.
    // */
    // virtual void add_arrival_time(float arrival_time);

    /**
     * \brief Add a packet to the send buffer at the indicated position
     */
    virtual int add_packet(Ptr<IoMusTPacket> packet);

    /**
     * \brief Retrieve the packet at the given position
     */
    virtual Ptr<IoMusTPacket> get_packet(int seqN);

    /**
     * \brief Marks the packet at the specified position as sent.
     */
    virtual void set_packet_sent(int seqN);
    
    // /**
    // * \brief Computes and returnes the jitter based on the arrival times previously stored in this instance.
    // */
    // virtual float get_jitter(void);

    /**
    * \brief Returns the address of the client associated with this stream.
    */
    virtual Address get_address(void);

    /**
    * \brief Sets the offset
    * 
    * \param offset to be applied to the sequence number to synchronized the stream
    */
    virtual void set_offset(int offset);

    /**
    * \brief Returns the offset to be applied to the sequence number to synchronized the stream
    */
    virtual int get_offset(void);

  protected:
    virtual void DoDelete (void);

  private:
    // /**
    //  * \brief Calculates the jitter from the arrival times in arrival_times
    //  */
    // virtual void calculate_jitter(void);

    //static constexpr int jitter_window_length = 50; //!< The length of array of arrival times
    static constexpr int array_size = 100; //!< The size of the array containing the send buffer

    Address address; //!< The client's ipv4 address
    //std::array<float, jitter_window_length> arrival_times; //!< Array of arrival times used to compute the jitter
    std::array<Ptr<IoMusTPacket>, array_size> send_buffer; //!< Array of packets waiting to be mixed
    int index = 0; //!< Highest index of the arrival_times circular buffer
    int offset;
    bool synchronized = false;
    //float jitter;
};

/**
 * \ingroup applications 
 * \defgroup udpmix UdpMix
 */

/**
 * \ingroup udpmix
 * \brief A Udp Mix server
 *
 * Every packet received is sent back.
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
  * \brief Sends all the given packets to the corresponding stream.
  */
  virtual void send_packets(int seq_n);

  /**
  * \brief Sends the given packets to the provided streams.
  */
  virtual void check_and_send_packets(int seq_n);

  /**
  * \brief Instanciates a stream and adds it to the send buffer.
  * 
  * \param client_address The address of the client associated with the stream.
  */
  virtual Ptr<Stream> add_stream(Address client_address, Ptr<IoMusTPacket> packet);

  /**
  * \brief Add the last packet that arrived.
  * 
  * \param from The address of the client that sent the packet.
  * \param packet The last packet that arrived.
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

  static constexpr int array_size = 1000;

  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<Socket> m_socket; //!< IPv4 Socket
  Ptr<Socket> m_socket6; //!< IPv6 Socket
  Address m_local; //!< local multicast address
  bool started = false; //!< flag that informs on the status of the server
  bool first_packet = true;
  std::vector<Stream> streams;
  Time reference_time = MicroSeconds(0);
  Time packet_transmission_period;
  Time timeout;
  Ptr<Socket> socket;
  std::array<IoMusTPacket, array_size> packet_storage;
  int index = 0;

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_Mix_SERVER_H */

