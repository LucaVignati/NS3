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

#include "udp-broadcast-server.h"
#include "seq-ts-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpBroadcastServerApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpBroadcastServer);

TypeId
UdpBroadcastServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpBroadcastServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpBroadcastServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&UdpBroadcastServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpBroadcastServer::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpBroadcastServer::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

UdpBroadcastServer::UdpBroadcastServer ()
{
  NS_LOG_FUNCTION (this);
}

UdpBroadcastServer::~UdpBroadcastServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket6 = 0;
}

void
UdpBroadcastServer::AddPeerAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_peerAddresses.push_back (address);
}

void
UdpBroadcastServer::SetPacketSize(int pkt_size)
{
  NS_LOG_FUNCTION (this << pkt_size);
  m_pktSize = pkt_size;
}

void
UdpBroadcastServer::SetMean(float mean)
{
  NS_LOG_FUNCTION (this << mean);
  m_mean = mean;
}

void
UdpBroadcastServer::SetStandardDeviation(float std_dev)
{
  NS_LOG_FUNCTION (this << std_dev);
  m_stdDev = std_dev;
}

void
UdpBroadcastServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
UdpBroadcastServer::StartApplication (void)
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
      distribution = std::normal_distribution<float>(m_mean, m_stdDev);
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

  m_socket->SetRecvCallback (MakeCallback (&UdpBroadcastServer::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&UdpBroadcastServer::HandleRead, this));
}

void 
UdpBroadcastServer::StopApplication ()
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
UdpBroadcastServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  Address to;
  Address localAddress;
  Time delay = MilliSeconds(distribution(generator));
  uint8_t *data;

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

      int pktSize = packet->GetSize();
      // std::cout << pktSize << std::endl;
      data = new uint8_t[m_pktSize];
      packet->CopyData(data, m_pktSize);
      Ipv4Address ip = Ipv4Address();
      ip = ip.Deserialize(data);
      to = InetSocketAddress(ip, data[4]);
      packet = Create<Packet> (data, m_pktSize);

      /*ip.Print(std::cout);
      std::cout << std::endl;*/

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      // std::cout << __LINE__ << std::endl;

      NS_LOG_LOGIC ("Broadcasting packet");
      for (Address to : m_peerAddresses)
      {
        if (InetSocketAddress::ConvertFrom (from).GetIpv4 () != InetSocketAddress::ConvertFrom (to).GetIpv4 ())
        {
          // std::cout << packet->GetSize() << std::endl;
          int (Socket::*fp)(Ptr<ns3::Packet>, uint32_t, const ns3::Address&) = &ns3::Socket::SendTo;
          Simulator::Schedule(delay, fp, socket, packet, 0, to);
        }
      }

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
