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

#include "udp-mix-mec-server.h"
#include "seq-ts-header.h"

// #define DEBUG
#define PORT 55


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpMixMecServerApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpMixMecServer);

TypeId
UdpMixMecServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpMixMecServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpMixMecServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&UdpMixMecServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TransmissionPeriod", "Time between the transmission of two consecutive packets.",
                   TimeValue (MilliSeconds (1/1500)),
                   MakeTimeAccessor(&UdpMixMecServer::packetTransmissionPeriod),
                   MakeTimeChecker())
    .AddAttribute ("Timeout", "Maximum time to wait for packets of the same slots to arrive",
                   TimeValue (MilliSeconds (10)),
                   MakeTimeAccessor(&UdpMixMecServer::timeout),
                   MakeTimeChecker())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpMixMecServer::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpMixMecServer::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

UdpMixMecServer::UdpMixMecServer ()
{
  NS_LOG_FUNCTION (this);
}

UdpMixMecServer::~UdpMixMecServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket6 = 0;
}

void
UdpMixMecServer::SetClientAddress (Address address)
{
  m_clientAddress = address;
}

void
UdpMixMecServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
UdpMixMecServer::StartApplication (void)
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
  m_socket->SetRecvCallback (MakeCallback (&UdpMixMecServer::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&UdpMixMecServer::HandleRead, this));
}

void 
UdpMixMecServer::StopApplication ()
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
UdpMixMecServer::send_packets(uint32_t normSeqN)
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
    p->set_pkt_size(p->get_pkt_size() * 2);
    p->set_seq_n(stream->convert_seq_n(normSeqN));
    int (Socket::*fp)(Ptr<ns3::Packet>, uint32_t, const ns3::Address&) = &ns3::Socket::SendTo;
    Simulator::Schedule(Seconds(0.0002), fp, socket, p->get_packet(), 0, m_clientAddress);
  }
  for (Ptr<Stream> stream : streams)
  {
    stream->set_packet_sent(normSeqN);
  }
#ifdef DEBUG 
    if (m_port == PORT)
      std::cout << std::endl << Simulator::Now().As(Time::S) << "\tSent slot " << normSeqN << std::endl;
#endif
}

void
UdpMixMecServer::check_and_send_packets(uint32_t normSeqN)
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
UdpMixMecServer::add_stream(Address clientAddress, Ptr<IoMusTPacket> packet)
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
    if (oldestSeqN == 0)
      oldestSeqN = newestSeqN;
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
UdpMixMecServer::add_packet(Ptr<IoMusTPacket> packet, Ptr<Stream> stream)
{
  uint32_t normSeqN = stream->add_packet(packet);
  if (normSeqN > newestSeqN) newestSeqN = normSeqN;

#ifdef DEBUG
  if (m_port == PORT)
    std::cout << "Norm Seq: " << normSeqN;
#endif

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
    Simulator::Schedule(timeout, &ns3::UdpMixMecServer::check_and_send_packets, this, normSeqN); // Schedule anyway, it will check if packets have already been sent in the meantime
#ifdef DEBUG    
    if (m_port == PORT)
      std::cout << std::endl;
#endif
  }
  else
  {
    stream->set_packet_sent(normSeqN);
#ifdef DEBUG    
    if (m_port == PORT)
      std::cout << "\tRejected" << std::endl;
#endif
    // Pacchetto in ritardo o perso in uplink
  }
}

void 
UdpMixMecServer::HandleRead (Ptr<Socket> socket)
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

#ifdef DEBUG
      if (m_port == PORT)
        std::cout << Simulator::Now().As(Time::S) << "\tReceived from ";
#endif

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
#ifdef DEBUG          
          if (m_port == PORT)
            std::cout << "[" << i << "]\t";
#endif
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

#ifdef DEBUG
      if (m_port == PORT)
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
#endif

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
