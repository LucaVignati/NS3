#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
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

void populate_positions(Ptr<ListPositionAllocator> positionAlloc, int numberOfueNodes, std::vector<Vector> enbs, int radius, double height, std::string filename, bool traffic)
{
    int enbIndex = 0;
    bool redo;
    Vector3D ue;
    std::vector<Vector> ues;
    std::ofstream positionsFile;
    positionsFile.open(filename.c_str(), std::ios_base::app);
    std::string trafficString;
    float spatialResolution = 0.1; // Meters
    int max_mod = static_cast<float>(2*radius)/spatialResolution;
    trafficString = "r";

    for (Vector enb : enbs)
        positionsFile << "eNB " << enb.x << " " << enb.y << " " << enb.z << " " << trafficString << std::endl;
    
    for (uint16_t i = 0; i < numberOfueNodes; i++)
    {
        redo = true;
        while (redo)
        {
            redo = false;
            ue.x = (rand() % max_mod) * spatialResolution + enbs[enbIndex].x - radius;
            ue.y = (rand() % max_mod) * spatialResolution + enbs[enbIndex].y - radius;
            ue.z = height;
            if (sqrt(pow(ue.x - enbs[enbIndex].x, 2) + pow(ue.y - enbs[enbIndex].y, 2)) > radius) // outside range
            {
                redo = true;
            }
            else
            {
                for (Vector u : ues)
                {
                    if (ue.x == u.x && ue.y == u.y && ue.z == u.z) // same position
                    {
                        redo = true;
                    }
                }
            }
        }
        ues.push_back(ue);
        positionAlloc->Add (ue);
        positionsFile << "UE " << ue.x << " " << ue.y << " " << ue.z << " " << trafficString << std::endl;
        enbIndex++;
    }
    positionsFile.close();
}

int
main (int argc, char *argv[])
{
    std::string generation = "4G";
    uint16_t scenario = 1;
    uint16_t numberOfueNodes = 2;
    double simTime = 2; // in seconds
    uint32_t radius = 500;
    double height = 1.5;
    double interPacketInterval = 1.0/1500.0;
    int nPackets;
    uint32_t audioPacketSize = 136;
    uint32_t htPacketSize = 24;
    uint16_t thrsLatency = 20*1000;
    uint32_t mixServerTimeout = 10;
    std::string comment = "";
    int seed = 0;
    int enbHeight = 25;
    int numerology = 2;
    bool shadowing = false;
    bool enableUlPc = true;
    double frequency = 2035e6; // central frequency
    double bwpBandwidth = 20e6; //bandwidth of the UL and the DL
    double spacingBandwidth = 170e6; // bandwidth of the bandwidth part used to separate the UL from the DL
    enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa; //UMi_Buildings
    std::string errorModel = "LenaErrorModel";

    float mean = 0;
    float stdDev = 0;

    bool traces = false;

    std::string path = "graphs";

    std::vector<std::vector<int>> combinations;
    std::vector<int> combination;

    Ptr<LteHelper> lteHelper;
    Ptr<NrHelper> nrHelper;
    Ptr<Node> pgw;
    NetDeviceContainer enbNetDevs;
    NetDeviceContainer ueNetDevs;
    Ptr<PointToPointEpcHelper> lteEpcHelper;
    Ptr<NrPointToPointEpcHelper> nrEpcHelper;
    BandwidthPartInfoPtrVector allBwps;
    OperationBandInfo band;
    MobilityHelper mobility;

    EpsBearer lowLatencyBearerUL (EpsBearer::GBR_CONV_VIDEO);
    EpsBearer lowLatencyBearerDL (EpsBearer::GBR_CONV_VOICE);
    Ptr<EpcTft> ulTft;
    Ptr<EpcTft> dlTft;

    Time::SetResolution (Time::NS);

    CommandLine cmd;
    cmd.AddValue("generation",
                "4G or 5G",
                generation);
    cmd.AddValue("scenario",
                "Scenario 1 or Scenario 2",
                scenario);
    cmd.AddValue("numberOfueNodes",
                "Number of UE nodes",
                numberOfueNodes);
    cmd.AddValue("comment",
                "Comment to be added in the filenames of the plots",
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
    cmd.AddValue("interPacketInterval",
                "Inter packet interval [s]",
                interPacketInterval);
    cmd.AddValue("nPackets",
                "Number of packets transmitted by each UE",
                nPackets);
    cmd.AddValue("numerology",
                "Value defining the subcarrier spaceing and symbol lenght",
                numerology);
    cmd.AddValue("path",
                "Path where the results will be saved",
                path);
    cmd.AddValue("shadowing",
                "Enable/Disable shadowing",
                shadowing);
    cmd.AddValue("traces",
                "Enable/Disable traces",
                traces);
    cmd.AddValue("errorModel",
                "Error model to be used",
                errorModel);
    cmd.AddValue("mean",
                "Mean of the normal distribution used to simulate the cloud",
                mean);
    cmd.AddValue("stdDev",
                "Standard deviation of the normal distribution used to simulate the cloud",
                stdDev);
    cmd.Parse(argc, argv);

    srand(seed);

    errorModel = "ns3::" + errorModel;

    char ues[10];
    sprintf(ues, "%d", numberOfueNodes);

    std::string root = path + "/simulations";
    std::string folder = root + "/" + ues + " UEs";
    std::string fileNamePrefix;


    fileNamePrefix = "r" + std::to_string(radius) + "-num" + std::to_string(numerology) + "-s" + std::to_string(seed) + "-t" + std::to_string(static_cast<int>(simTime)) + "-" + comment;
    /*
    * Default values for the simulation. We are progressively removing all
    * the instances of SetDefault, but we need it for legacy code (LTE)
    */
    //Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999)); // Either use it for both 4G and 5G or neither. Sandra suggests this could be the reason why we get such high latency in 5G.

    /*
    * Create NR simulation helpers
    */
    nrHelper = CreateObject<NrHelper> ();

    nrEpcHelper = CreateObject<NrPointToPointEpcHelper> ();
    nrHelper->SetEpcHelper (nrEpcHelper);

    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject <IdealBeamformingHelper> ();
    nrHelper->SetBeamformingHelper (idealBeamformingHelper);

    /*
    * Spectrum configuration. We create a single operational band and configure the scenario.
    */
    CcBwpCreator ccBwpCreator;
    
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
    band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);

    //For the case of manual configuration of CCs and BWPs
    std::unique_ptr<ComponentCarrierInfo> cc0 (new ComponentCarrierInfo ());
    std::unique_ptr<BandwidthPartInfo> bwp0 (new BandwidthPartInfo ());
    std::unique_ptr<BandwidthPartInfo> bwp1 (new BandwidthPartInfo ());
    std::unique_ptr<BandwidthPartInfo> bwp2 (new BandwidthPartInfo ());

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

    // BWP 0 - UL
    bwp0->m_bwpId = bwpCount;
    // bwp0->m_centralFrequency = cc0->m_lowerFrequency + bwpBandwidth/2;
    bwp0->m_centralFrequency = cc0->m_higherFrequency - bwpBandwidth/2;
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

    // BWP 2 - DL
    bwp2->m_bwpId = bwpCount;
    // bwp2->m_centralFrequency = cc0->m_higherFrequency - bwpBandwidth/2;
    bwp2->m_centralFrequency = cc0->m_lowerFrequency + bwpBandwidth/2;
    bwp2->m_channelBandwidth = bwpBandwidth;
    bwp2->m_lowerFrequency = bwp2->m_centralFrequency - bwp2->m_channelBandwidth / 2;
    bwp2->m_higherFrequency = bwp2->m_centralFrequency + bwp2->m_channelBandwidth / 2;

    cc0->AddBwp (std::move(bwp2));
    ++bwpCount;

    band.AddCc (std::move(cc0));

    nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (shadowing));
    nrHelper->SetUePhyAttribute ("EnableUplinkPowerControl", BooleanValue (enableUlPc));

    nrHelper->SetUlErrorModel(errorModel);
    nrHelper->SetDlErrorModel(errorModel);

    // nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel));
    // nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel));

    // Disable fast fading
    auto bandMask = NrHelper::INIT_PROPAGATION | NrHelper::INIT_CHANNEL;
    //Initialize channel and pathloss, plus other things inside band.
    nrHelper->InitializeOperationBand (&band, bandMask);
    allBwps = CcBwpCreator::GetAllBwps ({band});

    // Configure ideal beamforming method
    idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ())); //Not required if fast fading is not used --- IF OMITTED RESULTS IN SEGMENTATION FAULT DUE TO NULL POINTER

    nrEpcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));

    // Configure scheduler
    nrHelper->SetSchedulerTypeId (NrMacSchedulerTdmaPF::GetTypeId ()); // Proportional Fairness scheduler

    // nrHelper->SetSchedulerTypeId (NrMacSchedulerTdmaRR::GetTypeId ()); // Round Robin scheduler

    // Antennas for the UEs
    // nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
    // nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
    //nrHelper->SetUeAntennaAttribute ("IsotropicElements", BooleanValue (true));

    // Antennas for the gNbs
    // nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
    // nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
    //nrHelper->SetGnbAntennaAttribute ("IsotropicElements", BooleanValue (true));

    nrHelper->SetGnbPhyAttribute ("TxPower", DoubleValue (30));
    nrHelper->SetUePhyAttribute ("TxPower", DoubleValue (10));

    // gNb routing between Bearer and bandwidh part
    nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (0));
    nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VIDEO", UintegerValue (2));

    // UE routing between Bearer and bandwidh part
    nrHelper->SetUeBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (0));
    nrHelper->SetUeBwpManagerAlgorithmAttribute ("GBR_CONV_VIDEO", UintegerValue (2));

    pgw = nrEpcHelper->GetPgwNode ();

    double startTime = 0.1;
    double endTime = startTime + simTime + 7;
    double totalSimTime = endTime + 11;

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    Ptr<Node> remoteHost;
    remoteHostContainer.Create (1);
    remoteHost = remoteHostContainer.Get (0);

    InternetStackHelper internet;
    internet.Install (remoteHostContainer);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ipv4InterfaceContainer internetIpIfaces;
    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.00001)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    internetIpIfaces = ipv4h.Assign (internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
    
    std::cout << "Remote host address 0: " << internetIpIfaces.GetAddress (0) << std::endl;
    std::cout << "Remote host address 1: " << internetIpIfaces.GetAddress (1) << std::endl;

    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfueNodes);
    ueNodes.Create(numberOfueNodes);

    std::cout << "Number of UEs: " << ueNodes.GetN() << std::endl;

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    std::vector<Vector> enbs;
    int interCellDistance = 10000;
    for (int i = 0; i < numberOfueNodes; i++)
    {
        enbs.push_back(Vector(i*interCellDistance, 0, enbHeight));
    }

    std::string positionsFileName = folder + "/data/" + fileNamePrefix + "_positions.txt";
    std::ofstream ofs;
    ofs.open(positionsFileName, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
    populate_positions(positionAlloc, numberOfueNodes, enbs, radius, height, positionsFileName, false);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(ueNodes);

    positionAlloc = CreateObject<ListPositionAllocator> ();
    for (Vector enb : enbs)
    {
        positionAlloc->Add (enb);
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
 
    positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (enbs.at(0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(remoteHost);
    mobility.Install(pgw);

    // install nr net devices
    enbNetDevs = nrHelper->InstallGnbDevice(enbNodes, allBwps);
    ueNetDevs = nrHelper->InstallUeDevice (ueNodes, allBwps);

    int i;
    for(i = 0; i < numberOfueNodes; i++) {
        // BWP0, FDD-DL
        nrHelper->GetGnbPhy (enbNetDevs.Get (i), 0)->SetAttribute ("Numerology", UintegerValue (numerology));
        nrHelper->GetGnbPhy (enbNetDevs.Get (i), 0)->SetAttribute ("Pattern", StringValue ("DL|DL|DL|DL|DL|DL|DL|DL|DL|DL|"));
        nrHelper->GetGnbPhy (enbNetDevs.Get (i), 0)->SetTxPower (30);

        // BWP1, FDD-UL
        nrHelper->GetGnbPhy (enbNetDevs.Get (i), 2)->SetAttribute ("Numerology", UintegerValue (numerology));
        nrHelper->GetGnbPhy (enbNetDevs.Get (i), 2)->SetAttribute ("Pattern", StringValue ("UL|UL|UL|UL|UL|UL|UL|UL|UL|UL|"));
        nrHelper->GetGnbPhy (enbNetDevs.Get (i), 2)->SetTxPower (0);

        // Link the two FDD BWP:
        nrHelper->GetBwpManagerGnb (enbNetDevs.Get (i))->SetOutputLink (2, 0);
    }

    // Set the UE routing:
    for (uint32_t i = 0; i < ueNetDevs.GetN (); i++)
        {
        nrHelper->GetBwpManagerUe (ueNetDevs.Get (i))->SetOutputLink (0, 2);
        }

    // When all the configuration is done, explicitly call UpdateConfig ()
    for (auto it = enbNetDevs.Begin (); it != enbNetDevs.End (); ++it)
        {
        DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
        }

    for (auto it = ueNetDevs.Begin (); it != ueNetDevs.End (); ++it)
        {
        DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
        }

    // Install the IP stack on the UEs
    internet.Install (ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = nrEpcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDevs));

    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
        {
        Ptr<Node> ueNode = ueNodes.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (nrEpcHelper->GetUeDefaultGatewayAddress (), 1);
        }
    
    std::cout << nrEpcHelper->GetUeDefaultGatewayAddress () << std::endl;

    // attach UEs to the closest eNB
    nrHelper->AttachToClosestEnb (ueNetDevs, enbNetDevs);

    ulTft = Create<EpcTft> ();
    EpcTft::PacketFilter ulpfLowLatency;
    ulpfLowLatency.direction = EpcTft::UPLINK;
    ulTft->Add(ulpfLowLatency);

    dlTft = Create<EpcTft> ();
    EpcTft::PacketFilter dlpfLowLatency;
    dlpfLowLatency.direction = EpcTft::DOWNLINK;
    dlTft->Add(dlpfLowLatency);

    int numberOfdownlinkStreams = 1;
    if (scenario == 1)
    {
        numberOfdownlinkStreams = 1;
    }
    else if (scenario == 2)
    {
        numberOfdownlinkStreams = ueNodes.GetN() - 1;
    }

    uint64_t *e2eLatVect[ueNodes.GetN()][numberOfdownlinkStreams][2];
    for(uint32_t i = 0; i < ueNodes.GetN(); i++)
    {
        // if scenario 2
        for (int j = 0; j < numberOfdownlinkStreams; j++)
        {
            UdpIomustServerHelper udpServer(5 + j);
            uint64_t *arrivalTimeVect = (uint64_t *) malloc(nPackets * sizeof(uint64_t));
            uint64_t *latVect = (uint64_t *) malloc(nPackets * sizeof(uint64_t));
            for(int k = 0; k < nPackets; k++)
            {
                arrivalTimeVect[k] = 0;
                latVect[k] = 0;
            }
            e2eLatVect[i][j][0] = arrivalTimeVect;
            e2eLatVect[i][j][1] = latVect;
            ApplicationContainer ueServers = udpServer.Install(ueNodes.Get(i));
            Ptr<UdpIomustServer> server = udpServer.GetServer();
            server->SetMaxLatency(thrsLatency);
            server->SetDataVectors(arrivalTimeVect, latVect);
            server->SetNPackets(nPackets);
            ueServers.Start(Seconds(startTime));
            ueServers.Stop(Seconds(endTime));
        }
    }

    // Install and start applications on UEs and remote host
    if (scenario == 1)
    {
        for (int u = 0; u < numberOfueNodes; u++)
        {
            /*
            * Udp Mix Server
            * Listening on port 50
            * Sending to port 5 of the client address
            */
            UdpMixMecServerHelper udpMixMecServer(50 + u);
            udpMixMecServer.SetAttribute("TransmissionPeriod", TimeValue(Seconds(interPacketInterval)));
            udpMixMecServer.SetAttribute("Timeout", TimeValue(MilliSeconds(mixServerTimeout)));
            ApplicationContainer serverApps = udpMixMecServer.Install (remoteHost);
            Ipv4Address ip = ueIpIface.GetAddress(u);
            Address clientAddress = InetSocketAddress(ip, 5);
            serverApps.Get(0)->GetObject<UdpMixMecServer>()->SetClientAddress(clientAddress);
            serverApps.Start (Seconds (startTime));
            serverApps.Stop (Seconds (endTime));

            int port = 500 + u;

            UdpBroadcastServerHelper udpBroadcastServer(port);
            ApplicationContainer broadcastServerApps = udpBroadcastServer.Install (remoteHost);
            Ptr<UdpBroadcastServer> broadcastServer = broadcastServerApps.Get(0)->GetObject<UdpBroadcastServer>();
            broadcastServer->SetPacketSize(audioPacketSize);
            broadcastServer->SetMean(mean);
            broadcastServer->SetStandardDeviation(stdDev);
            
            for(uint16_t i = 1; i < numberOfueNodes; i++)
            {
                Address peerAddress = InetSocketAddress(remoteHostAddr, 50 + (u + i) % numberOfueNodes);
                std::cout << "UE " << i << " address: " << InetSocketAddress::ConvertFrom(peerAddress).GetIpv4() << " port: " << InetSocketAddress::ConvertFrom(peerAddress).GetPort() << std::endl;
                broadcastServerApps.Get(0)->GetObject<UdpBroadcastServer>()->AddPeerAddress(peerAddress);
            }
            broadcastServerApps.Start (Seconds (startTime));
            broadcastServerApps.Stop (Seconds (endTime));
        }

        for(uint16_t i = 0; i < numberOfueNodes; i++)
        {
            Address address = remoteHostAddr;
            int port = 500 + i;

            UdpIomustClientHelper udpClient(address, port);
            udpClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
            udpClient.SetAttribute ("Interval", TimeValue (Seconds (interPacketInterval)));
            ApplicationContainer clientApps = udpClient.Install(ueNodes.Get(i));
            uint8_t *data = new uint8_t[5];
            udpClient.SetFill(clientApps.Get(0), data, 5, audioPacketSize + htPacketSize);
            double sTime = (rand() % 500 + startTime*1000)/1000.0;
            clientApps.Start (Seconds (sTime));
            clientApps.Stop (Seconds (endTime));

            Ptr<Node> node = ueNodes.Get(i);
            Ptr<NetDevice> device = node->GetDevice(0);
            nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerUL, ulTft);
            nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerDL, dlTft);
        }
    }
    else if (scenario == 2)
    {
        for (int u = 0; u < numberOfueNodes; u++)
        {
            /*
            * Udp Mix Server
            * Listening on port 50
            * Sending to port 5 of the client address
            */
            // UdpMixServerHelper udpMixServer(50);
            // udpMixServer.SetAttribute("TransmissionPeriod", TimeValue(Seconds(interPacketInterval)));
            // udpMixServer.SetAttribute("Timeout", TimeValue(MilliSeconds(mixServerTimeout)));
            // ApplicationContainer serverApps = udpMixServer.Install (remoteHost);
            // Ipv4Address ip = ueIpIface.GetAddress(0);
            // Address clientAddress = InetSocketAddress(ip, 5);
            // serverApps.Get(0)->GetObject<UdpMixMecServer>()->SetClientAddress(clientAddress);
            // serverApps.Start (Seconds (startTime));
            // serverApps.Stop (Seconds (endTime));
            int port = 500 + u;

            UdpBroadcastServerHelper udpBroadcastServer(port);
            ApplicationContainer broadcastServerApps = udpBroadcastServer.Install (remoteHost);
            Ptr<UdpBroadcastServer> broadcastServer = broadcastServerApps.Get(0)->GetObject<UdpBroadcastServer>();
            broadcastServer->SetPacketSize(audioPacketSize);
            broadcastServer->SetMean(mean);
            broadcastServer->SetStandardDeviation(stdDev);
            
            for(uint16_t i = 1; i < numberOfueNodes; i++)
            {
                Ipv4Address address = ueIpIface.GetAddress ((u + i) % numberOfueNodes);
                Address peerAddress = InetSocketAddress(address, 5 + i - 1);
                std::cout << "UE " << i << " address: " << InetSocketAddress::ConvertFrom(peerAddress).GetIpv4() << " port: " << InetSocketAddress::ConvertFrom(peerAddress).GetPort() << std::endl;
                broadcastServerApps.Get(0)->GetObject<UdpBroadcastServer>()->AddPeerAddress(peerAddress);
            }
            broadcastServerApps.Start (Seconds (startTime));
            broadcastServerApps.Stop (Seconds (endTime));
        }

        for(uint16_t i = 0; i < numberOfueNodes; i++)
        {
            Address address = remoteHostAddr;
            int port = 500 + i;

            UdpIomustClientHelper udpClient(address, port);
            udpClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
            udpClient.SetAttribute ("Interval", TimeValue (Seconds (interPacketInterval)));
            ApplicationContainer clientApps = udpClient.Install(ueNodes.Get(i));
            uint8_t *data = new uint8_t[5];
            udpClient.SetFill(clientApps.Get(0), data, 5, audioPacketSize);
            double sTime = (rand() % 500 + startTime*1000)/1000.0;
            clientApps.Start (Seconds (sTime));
            clientApps.Stop (Seconds (endTime));

            Ptr<Node> node = ueNodes.Get(i);
            Ptr<NetDevice> device = node->GetDevice(0);
            nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerUL, ulTft);
            nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerDL, dlTft);
        }
    }
    

    // Flow monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.Install(ueNodes);
    flowHelper.Install(remoteHost);

    // Trace routing tables
    Ipv4GlobalRoutingHelper g;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>
    ("dynamic-global-routing.routes", std::ios::out);
    g.PrintRoutingTableAllAt (Seconds (2), routingStream);

    Simulator::Stop(Seconds(totalSimTime));
    Simulator::Run();

    std::string transferFileName = folder + "/data/" + fileNamePrefix + "_transferFile.txt";
    std::ofstream transferFile;
    transferFile.open(transferFileName.c_str(), std::ios_base::out | std::ios_base::trunc);

    Histogram e2eLatHist = Histogram(1);
    Histogram e2ePktHist = Histogram(1);
    uint64_t timeStamp;
    uint64_t val;
    int count = 0;
    int receivedPackets = 0;
    int latePackets = 0;
    int lostPackets = 0;
    double x;
    for(int i = 0; i < numberOfueNodes; i ++) {
        for (int j = 0; j < numberOfdownlinkStreams; j++)
        for(int k = 0; k < nPackets; k++) {
        timeStamp = (uint64_t) e2eLatVect[i][j][0][k];
        val = (uint64_t) e2eLatVect[i][j][1][k];
        if(val > 0)  {
            x = (double) val;
            if (val > 10000000) x = 10000000;
            receivedPackets++;
            e2eLatHist.AddValue(x);
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
        }
    }
    transferFile.close();

    int totalPackets = nPackets*numberOfueNodes;
    int deliveredPackets = receivedPackets;
    double deliveredPckPerc = (static_cast<double>(deliveredPackets) / (totalPackets)) * 100;
    double latePckPerc = (static_cast<double>(latePackets) / (totalPackets)) * 100;
    double lostPckPerc = (static_cast<double>(lostPackets) / (totalPackets)) * 100;
    double uselessPckPerc = latePckPerc + lostPckPerc;

    for(i = 0; i < numberOfueNodes; i++)  {
        for (int j = 0; j < numberOfdownlinkStreams; j++) {
            free(e2eLatVect[i][j][0]);
            free(e2eLatVect[i][j][1]);
        }
    }

    std::cout << "Delivered Packets: " << deliveredPckPerc << "%" << std::endl;
    std::cout << "Late Packets (> 20ms): " << latePckPerc << "%" << std::endl;
    std::cout << "Lost Packets (> 10s): " << lostPckPerc << "%" << std::endl;
    std::cout << "Packets Not In Time (Late + Lost): " << uselessPckPerc << "%" << std::endl;
    std::cout << "Delivered Packets: " << deliveredPackets << std::endl;
    std::cout << "Total Packets: " << totalPackets << std::endl;

    Simulator::Destroy();
    return 0;

    }