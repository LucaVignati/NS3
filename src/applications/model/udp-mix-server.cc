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

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "udp-mix-server.h"
#include "seq-ts-header.h"

namespace ns3 {

IoMusTPacket::IoMusTPacket() {}

IoMusTPacket::IoMusTPacket(Ptr<Packet> packet)
{
  uint64_t time;
  uint32_t seq_n;
  pktSize = packet->GetSize();
  data = new uint8_t[pktSize];
  packet->CopyData(data, pktSize);
  memcpy(&time, &data[5], sizeof(time));
  memcpy(&seq_n, &data[5 + sizeof(time)], sizeof(seq_n));
  seqN = static_cast<int>(seq_n);
  Ipv4Address ip = Ipv4Address();
  ip = ip.Deserialize(data);
  to = InetSocketAddress(ip, data[4]);
  sendTime = MicroSeconds(time);
}

IoMusTPacket::IoMusTPacket(Ptr<IoMusTPacket> packet)
{
  uint64_t time = 0;
  pktSize = packet->get_pkt_size();
  data = new uint8_t[pktSize];
  packet->copy_data(data);
  memcpy(&data[5], &time, sizeof(time));
}

IoMusTPacket::~IoMusTPacket()
{
  delete [] data;
}

Time
IoMusTPacket::get_send_time(void)
{
  return sendTime;
}

int
IoMusTPacket::get_seq_n(void)
{
  return seqN;
}

void
IoMusTPacket::set_seq_n(int seq_n)
{
  seqN = seq_n;
}

Ptr<Packet>
IoMusTPacket::get_packet(void)
{
  return Create<Packet> (data, pktSize);
}

Address
IoMusTPacket::get_to_address()
{
  return to;
}

void
IoMusTPacket::copy_data(uint8_t *data)
{
  memcpy(data, this->data, pktSize);
}

uint32_t
IoMusTPacket::get_pkt_size(void)
{
  return pktSize;
}

void
IoMusTPacket::DoDelete()
{
}

Stream::Stream (Address client_address)
{
  address = client_address;
  send_buffer.fill(NULL);
}

Stream::~Stream()
{
  send_buffer.fill(NULL);
}

int
Stream::add_packet(Ptr<IoMusTPacket> packet)
{
  int seq_n = packet->get_seq_n();
  int norm_seq_n = seq_n - offset;
  send_buffer[norm_seq_n % array_size] = packet;
  return norm_seq_n;
}

Ptr<IoMusTPacket>
Stream::get_packet(int seqN)
{
  return send_buffer[seqN % array_size];
}

void
Stream::set_packet_sent(int seq_n)
{
  send_buffer[seq_n % array_size] = NULL;
}

Address
Stream::get_address(void)
{
  return address;
}

void
Stream::set_offset(int o)
{
  offset = o;
}

int
Stream::get_offset(void)
{
  return offset;
}

int
Stream::convert_seq_n(int norm_seq_n)
{
  return norm_seq_n + offset;
}

void
Stream::DoDelete()
{
}

NS_LOG_COMPONENT_DEFINE ("UdpMixServerApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpMixServer);

TypeId
UdpMixServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpMixServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpMixServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&UdpMixServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TransmissionPeriod", "Time between the transmission of two consecutive packets.",
                   TimeValue (MilliSeconds (1/1500)),
                   MakeTimeAccessor(&UdpMixServer::packet_transmission_period),
                   MakeTimeChecker())
    .AddAttribute ("Timeout", "Maximum time to wait for packets of the same slots to arrive",
                   TimeValue (MilliSeconds (10)),
                   MakeTimeAccessor(&UdpMixServer::timeout),
                   MakeTimeChecker())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpMixServer::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpMixServer::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

UdpMixServer::UdpMixServer ()
{
  NS_LOG_FUNCTION (this);
}

UdpMixServer::~UdpMixServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket6 = 0;
}

void
UdpMixServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
UdpMixServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), m_port);
      if (m_socket6->Bind (local6) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      if (addressUtils::IsMulticast (local6))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket6);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, local6);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }
  m_socket->SetRecvCallback (MakeCallback (&UdpMixServer::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&UdpMixServer::HandleRead, this));
}

void 
UdpMixServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  if (m_socket6 != 0) 
    {
      m_socket6->Close ();
      m_socket6->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

}

void
UdpMixServer::send_packets(int seq_n)
{
  Ptr<IoMusTPacket> p;
  for (Ptr<Stream> stream : streams)
  {
    p = stream->get_packet(seq_n);
    if (p == NULL)
    {
      for (Ptr<Stream> s : streams)
      {
        p = s->get_packet(seq_n);
        if (p != NULL) break;
      }
      p = new IoMusTPacket(p);
    }
    p->set_seq_n(stream->convert_seq_n(seq_n));
    int (Socket::*fp)(Ptr<ns3::Packet>, uint32_t, const ns3::Address&) = &ns3::Socket::SendTo;
    Simulator::Schedule(Seconds(0.0002), fp, socket, p->get_packet(), 0, stream->get_address());
  }
  for (Ptr<Stream> stream : streams)
  {
    stream->set_packet_sent(seq_n);
  }
}

void
UdpMixServer::check_and_send_packets(int seq_n)
{
  Ptr<IoMusTPacket> p;
  bool already_sent = true;
  Ptr<Stream> stream;
  for (Ptr<Stream> stream : streams)
  {
    p = stream->get_packet(seq_n);
    if (p != NULL)
    {
      already_sent = false;
    }
  }
  if (!already_sent)
  {
    send_packets(seq_n);
  }
  oldest_seq_n = seq_n;
}

Ptr<Stream>
UdpMixServer::add_stream(Address client_address, Ptr<IoMusTPacket> packet)
{
  Time send_time = packet->get_send_time();
  int seq_n = packet->get_seq_n();
  Time stream_reference_time = send_time - seq_n * packet_transmission_period;
  int offset;
  streams.push_back(new Stream(client_address));
  Ptr<Stream> stream = streams.back();
  
  if (reference_time != MicroSeconds(0))
  {
    offset = round(static_cast<float>(reference_time.GetMicroSeconds() - stream_reference_time.GetMicroSeconds()) / packet_transmission_period.GetMicroSeconds());
  }
  else
  {
    reference_time = stream_reference_time;
    offset = 0;
  }
  stream->set_offset(offset);
  return stream;
}

void
UdpMixServer::add_packet(Ptr<IoMusTPacket> packet, Address from, Ptr<Stream> stream)
{
  int seq_n = stream->add_packet(packet);
  
  if (seq_n >= oldest_seq_n)
  {
    Ptr<IoMusTPacket> p;
    bool packets_ready = true;
    for (Ptr<Stream> stream : streams)
    {
      p = stream->get_packet(seq_n);
      if (p == NULL)
      {
        packets_ready = false;
        break;
      }
    }
    if (packets_ready)
    {
      send_packets(seq_n);
    }
    else
    {
      Simulator::Schedule(timeout, &ns3::UdpMixServer::check_and_send_packets, this, seq_n);
    }
  }
}

void 
UdpMixServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  this->socket = socket;
  Ptr<Packet> packet;
  Address from;
  Address to;
  Address localAddress;
  bool new_stream;
  //Time delay = Seconds(0);
  //std::cout << delay << std::endl;

  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server received " << packet->GetSize () << " bytes from " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }

      Ipv4Address ip = InetSocketAddress::ConvertFrom(from).GetIpv4();
      Address senderAddress = InetSocketAddress(ip, 5);

      Ptr<IoMusTPacket> packet_wrapper = new IoMusTPacket(packet);

      if (first_packet)
      {
        Ptr<Stream> stream = add_stream(senderAddress, packet_wrapper);
        first_packet = false;
      }
      new_stream = true;
      Ptr<Stream> stream;
      for (Ptr<Stream> stream : streams)
      {
        Address client_address = stream->get_address();
        if (client_address == senderAddress)
        {
          add_packet(packet_wrapper, senderAddress , stream);
          new_stream = false;
        }
      }
      if (new_stream)
      {
        Ptr<Stream> stream = add_stream(senderAddress, packet_wrapper);
        add_packet(packet_wrapper, senderAddress, stream);
      }

      /*ip.Print(std::cout);
      std::cout << std::endl;*/

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();



      NS_LOG_LOGIC ("Mixing packet");

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server sent " << packet->GetSize () << " bytes to " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server sent " << packet->GetSize () << " bytes to " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }
    }
}

} // Namespace ns3
