/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "udp-iomust-helper.h"
// #include "ns3/udp-iomust-server.h"
// #include "ns3/udp-iomust-client.h"
// #include "ns3/udp-mix-server.h"
// #include "ns3/udp-mix-mec-server.h"
// #include "ns3/udp-broadcast-server.h"
// #include "ns3/udp-forward-server.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

UdpBroadcastServerHelper::UdpBroadcastServerHelper (uint16_t port)
{
  m_factory.SetTypeId (UdpBroadcastServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
UdpBroadcastServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpBroadcastServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpBroadcastServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpBroadcastServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpBroadcastServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpBroadcastServer> ();
  node->AddApplication (app);

  return app;
}

UdpForwardServerHelper::UdpForwardServerHelper (uint16_t port)
{
  m_factory.SetTypeId (UdpForwardServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
UdpForwardServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpForwardServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpForwardServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpForwardServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpForwardServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpForwardServer> ();
  node->AddApplication (app);

  return app;
}

UdpMixServerHelper::UdpMixServerHelper (uint16_t port)
{
  m_factory.SetTypeId (UdpMixServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
UdpMixServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpMixServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpMixServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpMixServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpMixServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpMixServer> ();
  node->AddApplication (app);

  return app;
}

UdpMixMecServerHelper::UdpMixMecServerHelper (uint16_t port)
{
  m_factory.SetTypeId (UdpMixMecServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
UdpMixMecServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpMixMecServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpMixMecServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpMixMecServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpMixMecServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpMixMecServer> ();
  node->AddApplication (app);

  return app;
}

UdpIomustServerHelper::UdpIomustServerHelper ()
{
  m_factory.SetTypeId (UdpIomustServer::GetTypeId ());
}

UdpIomustServerHelper::UdpIomustServerHelper (uint16_t port)
{
  m_factory.SetTypeId (UdpIomustServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
UdpIomustServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpIomustServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<UdpIomustServer> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<UdpIomustServer>
UdpIomustServerHelper::GetServer (void)
{
  return m_server;
}

UdpIomustClientHelper::UdpIomustClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (UdpIomustClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

UdpIomustClientHelper::UdpIomustClientHelper (Address address)
{
  m_factory.SetTypeId (UdpIomustClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void 
UdpIomustClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
UdpIomustClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<UdpIomustClient>()->SetFill (fill);
}

void
UdpIomustClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<UdpIomustClient>()->SetFill (fill, dataLength);
}

void
UdpIomustClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<UdpIomustClient>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
UdpIomustClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpIomustClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpIomustClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpIomustClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpIomustClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
