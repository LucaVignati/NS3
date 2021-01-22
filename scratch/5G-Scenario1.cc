/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/**
 * \file cttc-3gpp-channel-example.cc
 * \ingroup examples
 * \brief Channel Example
 *
 * This example describes how to setup a simulation using the 3GPP channel model
 * from TR 38.900. Topology consists by default of 2 UEs and 2 gNbs, and can be
 * configured to be either mobile or static scenario.
 *
 * The output of this example are default NR trace files that can be found in
 * the root ns-3 project folder.
 */

#include "ns3/nr-helper.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/gnuplot.h"
#include <ns3/buildings-module.h>
#include <ns3/mobility-building-info.h>
#include <ns3/hybrid-buildings-propagation-loss-model.h>
#include <ns3/building.h>
#include <ns3/buildings-helper.h>
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include <stdlib.h>
#include <iostream>


using namespace ns3;



int 
main (int argc, char *argv[])
{

  uint16_t numberOfenbNodes = 1;
  uint16_t numberOfueNodes = 2;
  uint16_t numberOfBands = 1;
  double simTime = 2; // in seconds
  uint32_t squareWidth = 1000;
  uint32_t radius = 200;
  uint32_t height = 1;
  double interPacketInterval = 1.0/1500.0;
  uint32_t packetSize = 280;
  uint16_t thrsLatency = 20*1000;
  std::string comment = "";
  bool generateRem = false;
  int seed = 1;

  double frequency = 2035e6; // central frequency
  //double bandwidth = 40e6; //bandwidth (UL+DL)
  double bwpBandwidth = 20e6; //bandwidth of the UL and the DL
  double spacingBandwidth = 170e6; // bandwidth of the bandwidth part used to separate the UL from the DL
  int numerology = 2;
  
  
  enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::RMa; //UMi_Buildings

  CommandLine cmd;
  cmd.AddValue("numberOfueNodes",
                "Number of UE nodes",
                numberOfueNodes);
  cmd.AddValue("comment",
                "Comment to be added in the filenames of the plots.",
                comment);
  cmd.AddValue("radius",
                "Radius of the cirle in which the devices are randomly placed",
                radius);
  cmd.AddValue("height",
                "Height of the UEs",
                height);
  cmd.AddValue("seed",
                "Seed of the random function use to place the UEs",
                seed);
  cmd.AddValue("simTime",
                "Total duration of the simulation [s]",
                simTime);
  cmd.AddValue("numerology",
                "Value defining the subcarrier spaceing and symbol lenght",
                numerology);
  cmd.Parse (argc, argv);

  double startTime = 0.1;
  double endTime = startTime + simTime + 7;
  double totalSimTime = endTime + 11;
  int nPackets = simTime/interPacketInterval;

  char ues[10];
  sprintf(ues, "%d", numberOfueNodes); 

  numberOfBands = numberOfueNodes/2;
  uint16_t bands[numberOfBands];

  srand(seed);

  Time::SetResolution (Time::NS);

  /*
   * Default values for the simulation. We are progressively removing all
   * the instances of SetDefault, but we need it for legacy code (LTE)
   */
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));

  /*
   * Create NR simulation helpers
   */
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  nrHelper->SetEpcHelper (epcHelper);

  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject <IdealBeamformingHelper> ();
  nrHelper->SetIdealBeamformingHelper (idealBeamformingHelper);

  /*
   * Spectrum configuration. We create a single operational band and configure the scenario.
   */
  BandwidthPartInfoPtrVector allBwps;
  CcBwpCreator ccBwpCreator;

  //OperationBandInfo band;
  
  const uint8_t numCcPerBand = 1;  // in this example we have a single band, and that band is composed of a single component carrier

  /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
   * a single BWP per CC and a single BWP in CC.
   *
   * Hence, the configured spectrum is:
   *
   * |---------------------------------Band--------------------------------|
   * |----------------------------------CC---------------------------------|
   * |---------------BWP0---------------|---------------BWP1---------------|
   */
  CcBwpCreator::SimpleOperationBandConf bandConf (frequency, bwpBandwidth, numCcPerBand, scenarioEnum);
  bandConf.m_numBwp = 2; // Set the number of Bandwidth Parts to 2
  OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);

  //For the case of manual configuration of CCs and BWPs
  std::unique_ptr<ComponentCarrierInfo> cc0 (new ComponentCarrierInfo ());
  std::unique_ptr<BandwidthPartInfo> bwp0 (new BandwidthPartInfo ());
  std::unique_ptr<BandwidthPartInfo> bwp1 (new BandwidthPartInfo ());
  std::unique_ptr<BandwidthPartInfo> bwp2 (new BandwidthPartInfo ());

  std::cout << bwpBandwidth << std::endl;
  std::cout << spacingBandwidth << std::endl;
  std::cout << 2*bwpBandwidth + spacingBandwidth << std::endl;

  band.m_centralFrequency  = frequency;
  band.m_channelBandwidth = 2*bwpBandwidth + spacingBandwidth;
  band.m_lowerFrequency = band.m_centralFrequency - band.m_channelBandwidth / 2;
  band.m_higherFrequency = band.m_centralFrequency + band.m_channelBandwidth / 2;
  uint8_t bwpCount = 0;

  // Component Carrier 0
  cc0->m_ccId = 0;
  cc0->m_centralFrequency = frequency;
  cc0->m_channelBandwidth = 2*bwpBandwidth + spacingBandwidth;
  cc0->m_lowerFrequency = cc0->m_centralFrequency - cc0->m_channelBandwidth / 2;
  cc0->m_higherFrequency = cc0->m_centralFrequency + cc0->m_channelBandwidth / 2;

  // BWP 0 - DL
  bwp0->m_bwpId = bwpCount;
  bwp0->m_centralFrequency = cc0->m_lowerFrequency + bwpBandwidth/2;
  bwp0->m_channelBandwidth = bwpBandwidth;
  bwp0->m_lowerFrequency = bwp0->m_centralFrequency - bwp0->m_channelBandwidth / 2;
  bwp0->m_higherFrequency = bwp0->m_centralFrequency + bwp0->m_channelBandwidth / 2;

  cc0->AddBwp (std::move(bwp0));
  ++bwpCount;

  // BWP 1 - Spacing
  bwp1->m_bwpId = bwpCount;
  bwp1->m_centralFrequency = cc0->m_lowerFrequency + bwpBandwidth + spacingBandwidth/2;
  bwp1->m_channelBandwidth = spacingBandwidth;
  bwp1->m_lowerFrequency = bwp1->m_centralFrequency - bwp1->m_channelBandwidth / 2;
  bwp1->m_higherFrequency = bwp1->m_centralFrequency + bwp1->m_channelBandwidth / 2;

  cc0->AddBwp (std::move(bwp1));
  ++bwpCount;

  // BWP 2 - UL
  bwp2->m_bwpId = bwpCount;
  bwp2->m_centralFrequency = cc0->m_higherFrequency - spacingBandwidth/2;
  bwp2->m_channelBandwidth = bwpBandwidth;
  bwp2->m_lowerFrequency = bwp2->m_centralFrequency - bwp2->m_channelBandwidth / 2;
  bwp2->m_higherFrequency = bwp2->m_centralFrequency + bwp2->m_channelBandwidth / 2;

  cc0->AddBwp (std::move(bwp2));
  ++bwpCount;

  band.AddCc (std::move(cc0));

  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds(0)));
  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

  //Initialize channel and pathloss, plus other things inside band.
  nrHelper->InitializeOperationBand (&band);
  allBwps = CcBwpCreator::GetAllBwps ({band});

  // Configure ideal beamforming method
  idealBeamformingHelper->SetAttribute ("IdealBeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));

  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));

  // Configure scheduler
  nrHelper->SetSchedulerTypeId (NrMacSchedulerTdmaPF::GetTypeId ());

  // Antennas for the UEs
  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("IsotropicElements", BooleanValue (true));

  // Antennas for the gNbs
  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("IsotropicElements", BooleanValue (true));

  nrHelper->SetGnbPhyAttribute ("TxPower", DoubleValue (30));
  nrHelper->SetUePhyAttribute ("TxPower", DoubleValue (10));

  // gNb routing between Bearer and bandwidh part
  nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (0));
  nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VIDEO", UintegerValue (2));

  // UE routing between Bearer and bandwidh part
  nrHelper->SetUeBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (0));
  nrHelper->SetUeBwpManagerAlgorithmAttribute ("GBR_CONV_VIDEO", UintegerValue (2));

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  
  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.00001)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer bandNodes[numberOfBands];
  enbNodes.Create(numberOfenbNodes);
  for(uint32_t u = 0; u < numberOfBands; u++) {
    bands[u] = /*rand() % 3 +*/ 2;
    bandNodes[u].Create(bands[u]);
    ueNodes.Add(bandNodes[u]);
    std::cout << "Band " << u + 1 << ": " << bands[u] << std::endl;
  }

  NodeContainer trafficNode;
  trafficNode.Create(1);
  ueNodes.Add(trafficNode);

  //sprintf(comment, "%d", numberOfueNodes); 

  /*Ptr<GridBuildingAllocator>  gridBuildingAllocator;
  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (50));
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (17));
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (45));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (3));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (5));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (8));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (4));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("Type", EnumValue(Building::Residential));
  gridBuildingAllocator->SetBuildingAttribute ("ExternalWallsType", EnumValue(Building::ConcreteWithWindows));
  gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (0));
  gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (0));
  BuildingContainer buildings = gridBuildingAllocator->Create (1000);*/

  std::cout << "Number of UEs: " << ueNodes.GetN() << std::endl;

  // Install Mobility Model
  int v[numberOfueNodes][3] = {{0}};
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  int x_c, y_c, x, y, z;
  for (uint16_t i = 0; i < numberOfueNodes; i++)
    {
      bool redo = true;
      while (redo) {
        redo = false;
        x_c = squareWidth/2;
        y_c = squareWidth/2;
        int rho = rand() % radius +1;
        int theta = (rand() % 628 +1) / 100;
        x = static_cast<int>(rho * cos(theta));
        y = static_cast<int>(rho * sin(theta));
        int x_building = x/20;
        int y_building = y/50;
        x = x_building*20 + ((x%20) % 16) +1;
        y = (y_building*50 + ((x%50) % 44) +1);
        z = height;
        for (int j = 0; j < numberOfueNodes; j++) {
          if (x == v[j][0] && y == v[j][1] && z == v[j][2])
            redo = true;
        }
      }
      //int z = ((rand() % 2)*4)+1;
      v[i][0] = x;
      v[i][1] = y;
      v[i][2] = z;
      positionAlloc->Add (Vector(x + x_c, y + y_c, z));
      std::cout << "x: " << x + x_c << std::endl;
      std::cout << "y: " << y + y_c << std::endl;
      std::cout << "z: " << z << std::endl;
    }
  MobilityHelper mobility;
  //Ptr<RandomRoomPositionAllocator> buildingPositionAlloc = CreateObject<RandomRoomPositionAllocator> ();
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(ueNodes);
  //BuildingsHelper::Install(ueNodes);

  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector(squareWidth/4, squareWidth/2, 1));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(trafficNode);
  //BuildingsHlper::Install(trafficNode);

  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector(squareWidth/2, squareWidth/2, 20));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  //BuildingsHelper::Install(enbNodes);
  mobility.Install(remoteHost);
  //BuildingsHelper::Install(remoteHost);
  mobility.Install(pgw);
  //BuildingsHelper::Install(pgw);

  //BuildingsHelper::MakeMobilityModelConsistent ();

  // install nr net devices
  NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice(enbNodes, allBwps);
  NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes, allBwps);

  // BWP0, FDD-DL
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetAttribute ("Numerology", UintegerValue (numerology));
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetAttribute ("Pattern", StringValue ("DL|DL|DL|DL|DL|DL|DL|DL|DL|DL|"));
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetTxPower (30);

  // BWP1, FDD-UL
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 2)->SetAttribute ("Numerology", UintegerValue (numerology));
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 2)->SetAttribute ("Pattern", StringValue ("UL|UL|UL|UL|UL|UL|UL|UL|UL|UL|"));
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 2)->SetTxPower (0);

  // Link the two FDD BWP:
  nrHelper->GetBwpManagerGnb (enbNetDev.Get (0))->SetOutputLink (2, 0);

  // Set the UE routing:
  for (uint32_t i = 0; i < ueNetDev.GetN (); i++)
    {
      nrHelper->GetBwpManagerUe (ueNetDev.Get (i))->SetOutputLink (0, 2);
    }

  // When all the configuration is done, explicitly call UpdateConfig ()
  for (auto it = enbNetDev.Begin (); it != enbNetDev.End (); ++it)
    {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

  for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
    {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (ueNetDev, enbNetDev);

  // Install and start applications on UEs and remote host
  UdpForwardServerHelper udpForwardServer(9);
  ApplicationContainer serverApps = udpForwardServer.Install (remoteHost);
  serverApps.Start (Seconds (startTime));
  serverApps.Stop (Seconds (endTime));

  // Install and start traffic server on traffic UE
  Ptr<Node> node = trafficNode.Get(0);
  Ptr<NetDevice> trafficDevice = node->GetDevice(0);
  int32_t interface = node->GetObject<Ipv4>()->GetInterfaceForDevice(trafficDevice);
  Ipv4Address address = node->GetObject<Ipv4>()->GetAddress(interface, 0).GetLocal();
  UdpServerHelper udpTrafficServer(10);
  ApplicationContainer trafficServerApps = udpTrafficServer.Install(node);
  trafficServerApps.Start (Seconds (startTime));
  trafficServerApps.Stop (Seconds (endTime));

  int j;
  //uint64_t e2eLatVect[numberOfueNodes][nPackets];
  uint64_t *e2eLatVect[numberOfueNodes][2];
  UdpIomustServerHelper udpServer(5);
  for(int i = 0; i < numberOfueNodes; i++)  {
    uint64_t *arrivalTimeVect = (uint64_t *) malloc(nPackets * sizeof(uint64_t));
    uint64_t *latVect = (uint64_t *) malloc(nPackets * sizeof(uint64_t));
    for(j = 0; j < nPackets; j++) {
      arrivalTimeVect[j] = 0;
      latVect[j] = 0;
    }
    e2eLatVect[i][0] = arrivalTimeVect;
    e2eLatVect[i][1] = latVect;
    ApplicationContainer ueServers = udpServer.Install(ueNodes.Get(i));
    Ptr<UdpIomustServer> server = udpServer.GetServer();
    server->SetMaxLatency(thrsLatency);
    server->SetDataVectors(arrivalTimeVect, latVect);
    ueServers.Start(Seconds(startTime));
    ueServers.Stop(Seconds(endTime));
  }

  EpsBearer lowLatencyBearerUL (EpsBearer::GBR_CONV_VIDEO);
  EpsBearer lowLatencyBearerDL (EpsBearer::GBR_CONV_VOICE);

  Ptr<EpcTft> ulTft = Create<EpcTft> ();
  EpcTft::PacketFilter ulpfLowLatency;
  ulpfLowLatency.direction = EpcTft::UPLINK;
  ulTft->Add(ulpfLowLatency);

  Ptr<EpcTft> dlTft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpfLowLatency;
  dlpfLowLatency.direction = EpcTft::DOWNLINK;
  dlTft->Add(dlpfLowLatency);

  for(uint16_t u = 0; u < numberOfBands; u++) {
    for(uint16_t i = 0; i < bands[u]; i++)  {
      Ptr<Node> node = bandNodes[u].Get(i);
      Ptr<NetDevice> device = node->GetDevice(0);
      int32_t interface = node->GetObject<Ipv4>()->GetInterfaceForDevice(device);
      Ipv4Address address = node->GetObject<Ipv4>()->GetAddress(interface, 0).GetLocal();
      UdpIomustClientHelper udpClient(remoteHostAddr, 9);
      //address.Print(std::cout);
      //std::cout << std::endl;
      udpClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
      udpClient.SetAttribute ("Interval", TimeValue (Seconds (interPacketInterval)));
      for(uint16_t j = 1; j < bands[u]; j++)  {
        ApplicationContainer clientApps = udpClient.Install(bandNodes[u].Get((i + j) % bands[u]));
        uint8_t *data = new uint8_t[5];
        address.Serialize(data);
        data[4] = 5;
        //std::cout << unsigned(data[0]) << "." << unsigned(data[1]) << "." << unsigned(data[2]) << "." << unsigned(data[3]) << ":" << unsigned(data[4]) << std::endl;
        udpClient.SetFill(clientApps.Get(0), data, 5, packetSize);
        clientApps.Start (Seconds ((rand() % 100 + startTime*100)/100.0));
        clientApps.Stop (Seconds (endTime));
      }

      nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerUL, ulTft);
      nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerDL, dlTft);

    }
  }

  nrHelper->ActivateDedicatedEpsBearer (trafficDevice, lowLatencyBearerUL, ulTft);
  nrHelper->ActivateDedicatedEpsBearer (trafficDevice, lowLatencyBearerDL, dlTft);

  // Start traffic client on traffic remote host
  UdpClientHelper udpTrafficClient(address, 10);
  udpTrafficClient.SetAttribute("MaxPackets", UintegerValue(10000000));
  udpTrafficClient.SetAttribute("PacketSize", UintegerValue(14000)); // 14x10^3 bytes
  udpTrafficClient.SetAttribute ("Interval", TimeValue (Seconds (interPacketInterval)));
  ApplicationContainer trafficClientApps = udpTrafficClient.Install(remoteHost);
  trafficClientApps.Start (Seconds (startTime));
  trafficClientApps.Stop (Seconds (endTime));

  // enable the traces provided by the nr module
  nrHelper->EnableTraces();

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.Install(ueNodes);
  flowHelper.Install(remoteHost);

  AnimationInterface anim ("animation.xml");
  anim.SetMaxPktsPerTraceFile(5000000);
  anim.SetConstantPosition(remoteHost, squareWidth, squareWidth);

  Ptr<NrRadioEnvironmentMapHelper> remHelper;

  if (generateRem)
    {
      std::string filename = "eNB-REM.txt";
      std::ofstream outFile;
      outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
      if (!outFile.is_open ())
        {
          //NS_LOG_ERROR ("Can't open file " << filename);
          return -1;
        }
      for (NodeList::Iterator it = enbNodes.Begin (); it != enbNodes.End (); ++it)
        {
          Ptr<Node> node = *it;
          int nDevs = node->GetNDevices ();
          for (int j = 0; j < nDevs; j++)
            {
              Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
              if (enbdev)
                {
                  Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
                  outFile << "set label \"" << enbdev->GetCellId ()
                          << "\" at "<< pos.x << "," << pos.y
                          << " left font \"Helvetica,10\" textcolor rgb \"green\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                          << std::endl;
                }
            }
        }
      outFile.close();

      filename = "UE-REM.txt";
      outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
      if (!outFile.is_open ())
        {
          //NS_LOG_ERROR ("Can't open file " << filename);
          return -1;
        }
      for (NodeList::Iterator it = ueNodes.Begin (); it != ueNodes.End (); ++it)
        {
          Ptr<Node> node = *it;
          int nDevs = node->GetNDevices ();
          for (int j = 0; j < nDevs; j++)
            {
              Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
              if (uedev)
                {
                  Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
                  outFile << "set label \"" << uedev->GetImsi ()
                          << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,10\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
                          << std::endl;
                }
            }
        }
      outFile.close();

      /*filename = "buildings.txt";
      outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
      if (!outFile.is_open ())
        {
          //NS_LOG_ERROR ("Can't open file " << filename);
          return -1;
        }
      uint32_t index = 0;
      for (BuildingList::Iterator it = buildings.Begin (); it != buildings.End (); ++it)
        {
          ++index;
          Box box = (*it)->GetBoundaries ();
          outFile << "set object " << index
                  << " rect from " << box.xMin  << "," << box.yMin
                  << " to "   << box.xMax  << "," << box.yMax
                  << " front fs empty "
                  << std::endl;
        }*/

      remHelper = CreateObject<NrRadioEnvironmentMapHelper> ();
      /*remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/1"));
      remHelper->SetAttribute ("OutputFile", StringValue ("lena-mec.rem"));
      remHelper->SetAttribute ("XMin", DoubleValue (0));
      remHelper->SetAttribute ("XMax", DoubleValue (squareWidth));
      remHelper->SetAttribute ("YMin", DoubleValue (0));
      remHelper->SetAttribute ("YMax", DoubleValue (squareWidth));
      remHelper->SetAttribute ("Z", DoubleValue (1.8));
      remHelper->SetAttribute ("XRes", UintegerValue (500));
      remHelper->SetAttribute ("YRes", UintegerValue (500));
      remHelper->SetAttribute ("Bandwidth", UintegerValue (100));*/

      remHelper->SetMinX (0);
      remHelper->SetMaxX (squareWidth);
      remHelper->SetResX (500);
      remHelper->SetMinY (0);
      remHelper->SetMaxY (squareWidth);
      remHelper->SetResY (500);
      remHelper->SetZ (1.8);

      std::string null = "";
      std::string simTag = null + "" + ues + "-UEs";

      remHelper->SetSimTag(simTag);

      remHelper->CreateRem (enbNetDev, ueNetDev.Get(0), 0);

      /*if (remRbId >= 0)
        {
          remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));
          remHelper->SetAttribute ("RbId", IntegerValue (remRbId));
        }*/

      //remHelper->Install ();
      // simulation will stop right after the REM has been generated
    }

  Simulator::Stop (Seconds (totalSimTime));
  Simulator::Run ();

  // Print per flow statistics
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());

  std::map< FlowId, FlowMonitor::FlowStats > stats = flowMonitor->GetFlowStats ();

  int latePackets = 0;
  int lostPackets = 0;
  int deliveredPackets = 0;
  int totalPackets = 0;
  uint32_t maxLatency = 0;
  Histogram cumulativeUlHist(1);
  Histogram cumulativeDlHist(1);
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)  {

    Histogram h = iter->second.delayHistogram;
    for(uint32_t i = 0; i < h.GetNBins(); i++) {
      for(uint32_t j = 0; j < h.GetBinCount(i); j++)  {
        if(classifier->FindFlow(iter->first).destinationPort == 9)
          cumulativeUlHist.AddValue(i);
        else
          cumulativeDlHist.AddValue(i);
      }
      deliveredPackets += h.GetBinCount(i);
      if(i > 16)
        latePackets += h.GetBinCount(i);
      if(i > maxLatency)
        maxLatency = i;
    }
    lostPackets += iter->second.lostPackets;
  }

  Gnuplot2dDataset dataset;
  totalPackets = deliveredPackets + lostPackets;
  for(uint32_t i = 0; i < cumulativeUlHist.GetNBins(); i++)  {
    //dataset.Add(static_cast<double>(i)  , (static_cast<double>(cumulativeUlHist.GetBinCount(i))/(nPackets*numberOfueNodes))*100);
    dataset.Add(static_cast<double>(i)  , (static_cast<double>(cumulativeUlHist.GetBinCount(i))));
  }
  double deliveredPckPerc = (static_cast<double>(deliveredPackets) / (totalPackets)) * 100;
  double latePckPerc = (static_cast<double>(latePackets) / (totalPackets)) * 100;
  double lostPckPerc = (static_cast<double>(lostPackets) / (totalPackets)) * 100;
  double uselessPckPerc = latePckPerc + lostPckPerc;
  
  flowMonitor->SerializeToXmlFile("5G-Scenario1.xml", true, true);

  std::string root = "graphs/5G";
  std::string folder = root + "/" + ues + " UEs/";

  // GNU parameters
  
  std::string fileNamePrefix          = "r" + std::to_string(radius) + "-h" + std::to_string(height) + "-num" + std::to_string(numerology) + "-s" + std::to_string(seed) + "-t" + std::to_string(static_cast<int>(simTime)) + "-" + comment;
  std::string fileNameWithNoExtension = fileNamePrefix + "_UlLatencyHistogram";
  std::string graphicsFileName        = "../images/" + fileNameWithNoExtension + ".png";
  std::string plotFileName            = folder + "data/" + fileNameWithNoExtension + ".plt";
  std::string plotTitle               = "Latency Probability Histogram (Uplink)";
  std::string dataTitle               = "Latency";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot (graphicsFileName);
  gnuplot.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot.SetLegend ("Latency [ms]", "Probability [%]");

  dataset.SetTitle (dataTitle);
  dataset.SetStyle (Gnuplot2dDataset::IMPULSES);
  
  //Gnuplot ...continued
 
  gnuplot.AddDataset (dataset);

  // Open the plot file.
  //std::ofstream plotFile (plotFileName.c_str());

  // Write the plot file.
  //gnuplot.GenerateOutput (plotFile);

  // Close the plot file.
  //plotFile.close ();

  std::string transferFileName = folder + "/data/" + fileNamePrefix + "_transferFile.txt";
  std::ofstream transferFile;
  transferFile.open(transferFileName.c_str(), std::ios_base::out | std::ios_base::trunc);

  Histogram e2eLatHist = Histogram(1);
  Histogram e2ePktHist = Histogram(1);
  uint32_t val;
  uint32_t timeStamp;
  int count = 0;
  int receivedPackets = 0;
  latePackets = 0;
  lostPackets = 0;
  for(int i = 0; i < numberOfueNodes; i ++) {
    for(int j = 0; j < nPackets; j++) {
      timeStamp = static_cast<double>(e2eLatVect[i][0][j]);
      val = static_cast<double>(e2eLatVect[i][1][j]);
      if(val != 0)  {
        receivedPackets++;
        e2eLatHist.AddValue(val);
      }
      transferFile << i << " " << timeStamp << " " << val << std::endl;
      if(val == 0 || val > thrsLatency) {
        count++;
        if(val > thrsLatency)
          latePackets++;
        else
          lostPackets++;
      }
      else
        if(count > 0) {
          e2ePktHist.AddValue(static_cast<double>(count));
          count = 0;
        }
      //std::cout << i << "." << j << ": " << e2eLatVect[i][j] << std::endl;
    }
  }
  transferFile.close();

  totalPackets = nPackets*numberOfueNodes;
  deliveredPackets = receivedPackets;
  deliveredPckPerc = (static_cast<double>(deliveredPackets) / (totalPackets)) * 100;
  latePckPerc = (static_cast<double>(latePackets) / (totalPackets)) * 100;
  lostPckPerc = (static_cast<double>(lostPackets) / (totalPackets)) * 100;
  uselessPckPerc = latePckPerc + lostPckPerc;
  
  Gnuplot2dDataset dataset2;
  for(uint32_t i = 0; i < e2eLatHist.GetNBins(); i++)  {
    dataset2.Add(static_cast<double>(i)  , (static_cast<double>(e2eLatHist.GetBinCount(i))/receivedPackets)*100);
  }

  // GNU parameters
  fileNameWithNoExtension = fileNamePrefix + "_latencyHistogram";
  graphicsFileName        = "../images/" + fileNameWithNoExtension + ".png";
  plotFileName            = folder + "data/" + fileNameWithNoExtension + ".plt";
  plotTitle               = "End to End Latency Probability Histogram";
  dataTitle               = "Latency";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot2 (graphicsFileName);
  gnuplot2.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot2.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot2.SetLegend ("Latency [ms]", "Probability [%]");

  dataset2.SetTitle (dataTitle);
  dataset2.SetStyle (Gnuplot2dDataset::IMPULSES);
  
  //Gnuplot ...continued
 
  gnuplot2.AddDataset (dataset2);

  // Open the plot file.
  //std::ofstream plotFile2 (plotFileName.c_str());

  // Write the plot file.
  //gnuplot2.GenerateOutput (plotFile2);

  // Close the plot file.
  //plotFile2.close ();

  Gnuplot2dDataset dataset3;
  for(uint32_t i = 0; i < e2ePktHist.GetNBins(); i++)  {
    dataset3.Add(static_cast<double>(i)  , (static_cast<double>(e2ePktHist.GetBinCount(i))));
  }

  // GNU parameters
  fileNameWithNoExtension = fileNamePrefix + "_packetDropHistogram";
  graphicsFileName        = "../images/" + fileNameWithNoExtension + ".png";
  plotFileName            = folder + "data/" + fileNameWithNoExtension + ".plt";
  plotTitle               = "Consecutive Packet Drops Histogram";
  dataTitle               = "Consecutive Drops";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot3 (graphicsFileName);
  gnuplot3.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot3.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot3.SetLegend ("Number of consecutive drops", "Occurrence Frequency");

  dataset3.SetTitle (dataTitle);
  dataset3.SetStyle (Gnuplot2dDataset::IMPULSES);
  
  //Gnuplot ...continued
 
  gnuplot3.AddDataset (dataset3);

  // Open the plot file.
  //std::ofstream plotFile3 (plotFileName.c_str());

  // Write the plot file.
  //gnuplot3.GenerateOutput (plotFile3);

  // Close the plot file.
  //plotFile3.close ();

  Gnuplot2dDataset dataset4;
  for(uint32_t i = 0; i < cumulativeDlHist.GetNBins(); i++)  {
    dataset4.Add(static_cast<double>(i)  , (static_cast<double>(cumulativeDlHist.GetBinCount(i))/(nPackets*numberOfueNodes)*100));
  }

  // GNU parameters
  fileNameWithNoExtension = fileNamePrefix + "_DlLatencyHistogram";
  graphicsFileName        = "../images/" + fileNameWithNoExtension + ".png";
  plotFileName            = folder + "data/" + fileNameWithNoExtension + ".plt";
  plotTitle               = "Latency Probability Histogram (Downlink)";
  dataTitle               = "Latency";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot4 (graphicsFileName);
  gnuplot4.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot4.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot4.SetLegend ("Latency [ms]", "Probability [%]");

  dataset4.SetTitle (dataTitle);
  dataset4.SetStyle (Gnuplot2dDataset::IMPULSES);
  
  //Gnuplot ...continued
 
  gnuplot4.AddDataset (dataset4);

  // Open the plot file.
  //std::ofstream plotFile4 (plotFileName.c_str());

  // Write the plot file.
  //gnuplot4.GenerateOutput (plotFile4);

  // Close the plot file.
  //plotFile4.close ();

  Gnuplot2dDataset dataset5;
  double sum = 0;
  for(uint32_t i = 0; i < e2eLatHist.GetNBins(); i++)  {
    sum += (static_cast<double>(e2eLatHist.GetBinCount(i)*100)/totalPackets);
    dataset5.Add(static_cast<double>(i), sum);
  }

  // GNU parameters
  fileNameWithNoExtension = fileNamePrefix + "_latencyCumulative";
  graphicsFileName        = "../images/" + fileNameWithNoExtension + ".png";
  plotFileName            = folder + "data/" + fileNameWithNoExtension + ".plt";
  plotTitle               = "End to End Latency Cumulative Distribution";
  dataTitle               = "Latency";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot5 (graphicsFileName);
  gnuplot5.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot5.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot5.SetLegend ("Latency [ms]", "Probability [%]");

  dataset5.SetTitle (dataTitle);
  dataset5.SetStyle (Gnuplot2dDataset::LINES);
  
  //Gnuplot ...continued
 
  gnuplot5.AddDataset (dataset5);

  // Open the plot file.
  //std::ofstream plotFile5 (plotFileName.c_str());

  // Write the plot file.
  //gnuplot5.GenerateOutput (plotFile5);

  // Close the plot file.
  //plotFile5.close ();

  std::cout << "Delivered Packets: " << deliveredPckPerc << "%" << std::endl;
  std::cout << "Late Packets (> 20ms): " << latePckPerc << "%" << std::endl;
  std::cout << "Lost Packets (> 10s): " << lostPckPerc << "%" << std::endl;
  std::cout << "Packets Not In Time (Late + Lost): " << uselessPckPerc << "%" << std::endl;
  std::cout << "Delivered Packets: " << deliveredPackets << std::endl;
  //std::cout << "Late Packets: " << latePackets << std::endl;
  //std::cout << "Lost Packets: " << lostPackets << std::endl;
  std::cout << "Total Packets: " << totalPackets << std::endl;


  Simulator::Destroy ();
  return 0;
}


