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
  int64_t time;
  pktSize = packet->GetSize();
  data = new uint8_t[pktSize];
  packet->CopyData(data, pktSize);
  memcpy(&time, &data[5], sizeof(time));
  memcpy(&seqN, &data[5 + sizeof(time)], sizeof(seqN));
  Ipv4Address ip = Ipv4Address();
  ip = ip.Deserialize(data);
  to = InetSocketAddress(ip, data[4]);
  sendTime = MicroSeconds(time);
  arrivalTime = Simulator::Now();
}

IoMusTPacket::IoMusTPacket(Ptr<IoMusTPacket> packet)
{
  uint64_t time = 0;
  pktSize = packet->get_pkt_size();
  data = new uint8_t[pktSize];
  packet->copy_data(data);
  memcpy(&data[5], &time, sizeof(time));
  arrivalTime = Simulator::Now() + MilliSeconds(1);
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

uint32_t
IoMusTPacket::get_seq_n(void)
{
  return seqN;
}

void
IoMusTPacket::set_seq_n(uint32_t seqN)
{
  this->seqN = seqN;
  memcpy(&data[5 + sizeof(int64_t)], &this->seqN, sizeof(this->seqN));
}

Ptr<Packet>
IoMusTPacket::get_packet(void)
{
  Ptr<Packet> packet = Create<Packet> (data, pktSize);
  // int stayTime = Simulator::Now().GetMilliSeconds() - arrivalTime.GetMilliSeconds();
  // std::cout << stayTime << std::endl;
  // Ptr<IoMusTPacket> packetWrapper = new IoMusTPacket(packet);
  // uint32_t seqN = packetWrapper->get_seq_n();
  // std::cout << "seqN: " << seqN << std::endl;
  return packet;
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

Stream::Stream (Address clientAddress)
{
  address = clientAddress;
  sendBuffer.fill(NULL);
}

Stream::~Stream()
{
  sendBuffer.fill(NULL);
}

int
Stream::add_packet(Ptr<IoMusTPacket> packet)
{
  int seqN = packet->get_seq_n();
  int normSeqN = seqN - offset + OFFSET;
  int idx = normSeqN % arraySize;
  sendBuffer[idx] = packet;
  return normSeqN;
}

Ptr<IoMusTPacket>
Stream::get_packet(uint32_t seqN)
{
  return sendBuffer[seqN % arraySize];
}

void
Stream::set_packet_sent(uint32_t normSeqN)
{
  sendBuffer[normSeqN % arraySize] = NULL;
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

uint32_t
Stream::convert_seq_n(uint32_t normSeqN)
{
  return static_cast<uint32_t>(normSeqN - offset - OFFSET);
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
                   MakeTimeAccessor(&UdpMixServer::packetTransmissionPeriod),
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
UdpMixServer::send_packets(uint32_t normSeqN)
{
  Ptr<IoMusTPacket> p;
  for (Ptr<Stream> stream : streams)
  {
    p = stream->get_packet(normSeqN);
    if (p == NULL)
    {
      for (Ptr<Stream> s : streams)
      {
        p = s->get_packet(normSeqN);
        if (p != NULL) break;
      }
      p = new IoMusTPacket(p);
    }
    //std::cout << "normSeqN: " << normSeqN << std::endl;
    p->set_seq_n(stream->convert_seq_n(normSeqN));
    int (Socket::*fp)(Ptr<ns3::Packet>, uint32_t, const ns3::Address&) = &ns3::Socket::SendTo;
    Simulator::Schedule(Seconds(0.0002), fp, socket, p->get_packet(), 0, stream->get_address());
  }
  for (Ptr<Stream> stream : streams)
  {
    stream->set_packet_sent(normSeqN);
  }
  if (m_port == selectionPort)
    std::cout << std::endl << Simulator::Now().As(Time::S) << "\tSent slot " << normSeqN << std::endl;
}

void
UdpMixServer::check_and_send_packets(uint32_t normSeqN)
{
  Ptr<IoMusTPacket> p;
  bool alreadySent = true;
  for (Ptr<Stream> stream : streams)
  {
    p = stream->get_packet(normSeqN);
    if (p != NULL)
    {
      alreadySent = false;
    }
  }
  if (!alreadySent)
  {
    send_packets(normSeqN);
  }
  oldestSeqN = normSeqN;
}

Ptr<Stream>
UdpMixServer::add_stream(Address clientAddress, Ptr<IoMusTPacket> packet)
{
  Time sendTime = packet->get_send_time();
  int seqN = packet->get_seq_n();
  Time streamStartTime = sendTime - seqN * packetTransmissionPeriod;
  int offset;
  streams.push_back(new Stream(clientAddress));
  Ptr<Stream> stream = streams.back();
  
  if (reference_time != MicroSeconds(0))
  {
    offset = round(static_cast<float>(reference_time.GetMicroSeconds() - streamStartTime.GetMicroSeconds()) / packetTransmissionPeriod.GetMicroSeconds());
  }
  else
  {
    reference_time = streamStartTime;
    offset = 0;
  }
  stream->set_offset(offset);
  return stream;
}

void
UdpMixServer::add_packet(Ptr<IoMusTPacket> packet, Ptr<Stream> stream)
{
  uint32_t normSeqN = stream->add_packet(packet);
  if (normSeqN > newestSeqN) newestSeqN = normSeqN;
  
  if (m_port == selectionPort)
    std::cout << "Norm Seq: " << normSeqN;

  if (normSeqN > oldestSeqN)
  {
    Ptr<IoMusTPacket> p;
    bool packetsReady = true;
    for (Ptr<Stream> s : streams)
    {
      p = s->get_packet(normSeqN);
      if (p == NULL)
      {
        packetsReady = false;
        break;
      }
    }
    if (packetsReady)
    {
      send_packets(normSeqN);
    }
    Simulator::Schedule(timeout, &ns3::UdpMixServer::check_and_send_packets, this, normSeqN); // Schedule anyway, it will check if packets have already been sent in the meantime
    if (m_port == selectionPort)
      std::cout << std::endl;
  }
  else
  {
    stream->set_packet_sent(normSeqN);
    if (m_port == selectionPort)
      std::cout << "\tRejected" << std::endl;
    // Pacchetto in ritardo o perso in uplink
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
  bool newStream;

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

      if (m_port == selectionPort)
        std::cout << Simulator::Now().As(Time::S) << "\tReceived from ";

      Ptr<IoMusTPacket> packetWrapper = new IoMusTPacket(packet);

      if (firstPacket)
      {
        Ptr<Stream> stream = add_stream(senderAddress, packetWrapper);
        firstPacket = false;
      }
      newStream = true;
      int i = 0;
      for (Ptr<Stream> stream : streams)
      {
        Address clientAddress = stream->get_address();
        if (clientAddress == senderAddress)
        {
          if (m_port == selectionPort)
            std::cout << "[" << i << "]\t";
          add_packet(packetWrapper, stream);
          newStream = false;
        }
        i++;
      }
      if (newStream)
      {
        Ptr<Stream> stream = add_stream(senderAddress, packetWrapper);
        add_packet(packetWrapper, stream);
      }

      if (m_port == selectionPort)
      {
        i = 0;
        std::cout << "Streams\t";
        for (Ptr<Stream> stream : streams)
        {
          std::cout << "[" << i << "]\t";
          i++;
        }
        std::cout << std::endl;

        for (i = 0; i < 100; i++)
        {
          std::cout << newestSeqN - i << "\t ";
          for (Ptr<Stream> stream : streams)
          {
            int present = stream->get_packet(newestSeqN - i) == NULL ? 0 : 1;
            std::cout << present << "\t ";
          }
          std::cout << -i << std::endl;
        }
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
