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
#ifndef UDP_IOMUST_HELPER_H
#define UDP_IOMUST_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/udp-iomust-server.h"
#include "ns3/udp-mix-server.h"

namespace ns3 {

/**
 * \ingroup udpiomust
 * \brief Create a server application which waits for input UDP packets
 *        and sends them back to the original sender.
 */
class UdpForwardServerHelper
{
public:
  /**
   * Create UdpForwardServerHelper which will make life easier for people trying
   * to set up simulations with iomust devices.
   *
   * \param port The port the server will wait on for incoming packets
   */
  UdpForwardServerHelper (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a UdpForwardServerApplication on the specified Node.
   *
   * \param node The node on which to create the Application.  The node is
   *             specified by a Ptr<Node>.
   *
   * \returns An ApplicationContainer holding the Application created,
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a UdpForwardServerApplication on specified node
   *
   * \param nodeName The node on which to create the application.  The node
   *                 is specified by a node name previously registered with
   *                 the Object Name Service.
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   *
   * Create one udp forward server application on each of the Nodes in the
   * NodeContainer.
   *
   * \returns The applications created, one Application per Node in the 
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c) const;

private:
  /**
   * Install an ns3::UdpForwardServer on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an UdpForwardServer will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.
};

/**
 * \ingroup udpiomust
 * \brief Create a server application which waits for input UDP packets
 *        and sends them back to the original sender.
 */
class UdpMixServerHelper
{
public:
  /**
   * Create UdpMixServerHelper which will make life easier for people trying
   * to set up simulations with iomust devices.
   *
   * \param port The port the server will wait on for incoming packets
   */
  UdpMixServerHelper (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a UdpMixServerApplication on the specified Node.
   *
   * \param node The node on which to create the Application.  The node is
   *             specified by a Ptr<Node>.
   *
   * \returns An ApplicationContainer holding the Application created,
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a UdpMixServerApplication on specified node
   *
   * \param nodeName The node on which to create the application.  The node
   *                 is specified by a node name previously registered with
   *                 the Object Name Service.
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   *
   * Create one udp mix server application on each of the Nodes in the
   * NodeContainer.
   *
   * \returns The applications created, one Application per Node in the 
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c) const;

private:
  /**
   * Install an ns3::UdpMixServer on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an UdpMixServer will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.
};

class UdpIomustServerHelper
{
public:
  /**
   * Create UdpIomustServerHelper which will make life easier for people trying
   * to set up simulations with udp-iomust-server application.
   *
   */
  UdpIomustServerHelper ();

  /**
   * Create UdpIomustServerHelper which will make life easier for people trying
   * to set up simulations with udp-iomust-server application.
   *
   * \param port The port the server will wait on for incoming packets
   */
  UdpIomustServerHelper (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP iomust server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

  /**
   * \brief Return the last created server.
   *
   * This function is mainly used for testing.
   *
   * \returns a Ptr to the last created server application
   */
  Ptr<UdpIomustServer> GetServer (void);
private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<UdpIomustServer> m_server; //!< The last created server application
};

/**
 * \ingroup udpiomust
 * \brief Create an application which sends a UDP packet
 */
class UdpIomustClientHelper
{
public:
  /**
   * Create UdpIomustClientHelper which will make life easier for people trying
   * to set up simulations with iomust devices. Use this variant with addresses that do
   * not include a port value (e.g., Ipv4Address and Ipv6Address).
   *
   * \param ip The IP address of the remote udp iomust server
   * \param port The port number of the remote udp iomust server
   */
  UdpIomustClientHelper (Address ip, uint16_t port);
  /**
   * Create UdpIomustClientHelper which will make life easier for people trying
   * to set up simulations with iomust devices. Use this variant with addresses that do
   * include a port value (e.g., InetSocketAddress and Inet6SocketAddress).
   *
   * \param addr The address of the remote udp iomust server
   */
  UdpIomustClientHelper (Address addr);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Given a pointer to a UdpIomustClient application, set the data fill of the 
   * packet (what is sent as data to the server) to the contents of the fill
   * string (including the trailing zero terminator).
   *
   * \warning The size of resulting packets will be automatically adjusted
   * to reflect the size of the fill string -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param app Smart pointer to the application (real type must be UdpIomustClient).
   * \param fill The string to use as the actual data bytes.
   */
  void SetFill (Ptr<Application> app, std::string fill);

  /**
   * Given a pointer to a UdpIomustClient application, set the data fill of the 
   * packet (what is sent as data to the server) to the contents of the fill
   * byte.
   *
   * The fill byte will be used to initialize the contents of the data packet.
   * 
   * \warning The size of resulting packets will be automatically adjusted
   * to reflect the dataLength parameter -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param app Smart pointer to the application (real type must be UdpIomustClient).
   * \param fill The byte to be repeated in constructing the packet data..
   * \param dataLength The desired length of the resulting packet data.
   */
  void SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength);

  /**
   * Given a pointer to a UdpIomustClient application, set the data fill of the 
   * packet (what is sent as data to the server) to the contents of the fill
   * buffer, repeated as many times as is required.
   *
   * Initializing the fill to the contents of a single buffer is accomplished
   * by providing a complete buffer with fillLength set to your desired 
   * dataLength
   *
   * \warning The size of resulting packets will be automatically adjusted
   * to reflect the dataLength parameter -- this means that the PacketSize
   * attribute of the Application may be changed as a result of this call.
   *
   * \param app Smart pointer to the application (real type must be UdpIomustClient).
   * \param fill The fill pattern to use when constructing packets.
   * \param fillLength The number of bytes in the provided fill pattern.
   * \param dataLength The desired length of the final data.
   */
  void SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength);

  /**
   * Create a udp iomust client application on the specified node.  The Node
   * is provided as a Ptr<Node>.
   *
   * \param node The Ptr<Node> on which to create the UdpIomustClientApplication.
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a udp iomust client application on the specified node.  The Node
   * is provided as a string name of a Node that has been previously 
   * associated using the Object Name Service.
   *
   * \param nodeName The name of the node on which to create the UdpIomustClientApplication
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c the nodes
   *
   * Create one udp iomust client application on each of the input nodes
   *
   * \returns the applications created, one application per input node.
   */
  ApplicationContainer Install (NodeContainer c) const;

private:
  /**
   * Install an ns3::UdpIomustClient on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an UdpIomustClient will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* UDP_IOMUST_HELPER_H */
