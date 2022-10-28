#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
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
    int enbIndex; //, rho;
    //double theta;
    bool redo;
    Vector3D ue;
    std::vector<Vector> ues;
    std::ofstream positionsFile;
    positionsFile.open(filename.c_str(), std::ios_base::app);
    std::string trafficString;
    float spatialResolution = 0.1; // Meters
    int max_mod = static_cast<float>(2*radius)/spatialResolution;
    if (traffic)
        trafficString = "t";
    else
        trafficString = "r";

    for (Vector enb : enbs)
        positionsFile << "eNB " << enb.x << " " << enb.y << " " << enb.z << " " << trafficString << std::endl;
    
    for (uint16_t i = 0; i < numberOfueNodes; i++)
    {
        redo = true;
        while (redo)
        {
            redo = false;
            enbIndex = rand() % enbs.size();
            ue.x = (rand() % max_mod) * spatialResolution + enbs[enbIndex].x - radius;
            ue.y = (rand() % max_mod) * spatialResolution + enbs[enbIndex].y - radius;
            // rho = rand() % radius + 1;
            // theta = static_cast<double>(rand() % 628) / 100;
            // ue.x = static_cast<int>(rho * cos(theta) + enbs[enbIndex].x);
            // ue.y = static_cast<int>(rho * sin(theta) + enbs[enbIndex].y);
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
    }
    positionsFile.close();
}

void calculate_combinations(std::vector<std::vector<int>> *combinations, std::vector<int> *combination, int sum)
{
    int partial = sum;
    if(combination->size() > 0)
        if (partial > combination->back())
            partial = combination->back();
    std::vector<int> comb;
    for (int n : *combination)
        comb.push_back(n);
    while (true)
    {
        if (sum == 2)
        {
            comb.push_back(partial);
            combinations->push_back(comb);
            break;
        }
        else if (sum == 1)
            break;
        else if (sum == 0)
        {
            combinations->push_back(comb);
            break;
        }
        if (partial < 2)
            break;
        
        comb.push_back(partial);
        calculate_combinations(combinations, &comb, sum - partial);
        comb.pop_back();
        partial--;
    }
}

int
main (int argc, char *argv[])
{
    bool five_g;
    bool traffic = false;
    std::string generation = "4G";
    uint16_t scenario = 1;
    uint16_t numberOfenbNodes = 1;
    uint16_t numberOfueNodes = 2;
    uint16_t numberOfBands = 1;
    double simTime = 2; // in seconds
    uint32_t squareWidth = 1000;
    uint32_t radius = 500;
    double height = 1.5;
    double interPacketInterval = 1.0/1500.0;
    int nPackets;
    uint32_t packetSize = 280;
    uint16_t thrsLatency = 20*1000;
    uint32_t mixServerTimeout = 10;
    std::string comment = "";
    int seed = 0;
    float multiplier = 2;
    int enbHeight = 25;
    int numerology = 2;
    bool shadowing = true;
    bool enableUlPc = true;
    double frequency = 2035e6; // central frequency
    double bwpBandwidth = 20e6; //bandwidth of the UL and the DL
    double spacingBandwidth = 170e6; // bandwidth of the bandwidth part used to separate the UL from the DL
    enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa; //UMi_Buildings

    std::string path = "graphs";

    std::vector<std::vector<int>> combinations;
    std::vector<int> combination;

    int trafficUeNodesPerenb = 10;
    int numberOfTrafficEnbNodes = 0;

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
    cmd.AddValue("traffic",
                "Switches on the background internet traffic",
                traffic);
    cmd.AddValue("numberOfenbNodes",
                "Number of eNodeBs",
                numberOfenbNodes);
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
    cmd.AddValue("trafficDistanceMultiplier",
                "Distance between the traffic eNBs and the user eNBs",
                multiplier);
    cmd.AddValue("shadowing",
                "Enable/Disable shadowing",
                shadowing)
    cmd.Parse(argc, argv);

    srand(seed);

    int cell2cellDistance = 500*multiplier;

    five_g = generation.compare("5G") == 0;

    char ues[10];
    sprintf(ues, "%d", numberOfueNodes);

    if (traffic)
    {
        if (scenario == 1)
            numberOfTrafficEnbNodes = 6;
        else if (scenario == 2)
            numberOfTrafficEnbNodes = 9;
    }

    calculate_combinations(&combinations, &combination, numberOfueNodes);
    combination = combinations.at(rand() % combinations.size());
    numberOfBands = combination.size();
    uint16_t bands[numberOfBands];

    std::string root = path + "/simulations";
    std::string folder = root + "/" + ues + " UEs/";
    std::string fileNamePrefix;

    std::cout << folder << std::endl;

    if (scenario == 1)
    {
        if (five_g) // *** 5G ***
            fileNamePrefix = "r" + std::to_string(radius) + "-num" + std::to_string(numerology) + "-s" + std::to_string(seed) + "-t" + std::to_string(static_cast<int>(simTime)) + "-" + comment;
        else // *** 4G ***
            fileNamePrefix = "r" + std::to_string(radius) + "-s" + std::to_string(seed) + "-t" + std::to_string(static_cast<int>(simTime)) + "-" + comment;
    }
    else if (scenario == 2)
    {
        numberOfenbNodes = 3;
        if (five_g) // *** 5G ***
            fileNamePrefix = "gNB" + std::to_string(numberOfenbNodes) + "-r" + std::to_string(radius) + "-num" + std::to_string(numerology) + "-s" + std::to_string(seed) + "-t" + std::to_string(static_cast<int>(simTime)) + "-" + comment;
        else // *** 4G ***
            fileNamePrefix = "eNB" + std::to_string(numberOfenbNodes) + "-r" + std::to_string(radius) + "-s" + std::to_string(seed) + "-t" + std::to_string(static_cast<int>(simTime)) + "-" + comment;
    }

    if(five_g)
    { // *** 5G ***
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

        nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (shadowing));
        nrHelper->SetUePhyAttribute ("EnableUplinkPowerControl", BooleanValue (enableUlPc));

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
    }
    else
    { // *** 4G ***
        Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (100));   // 2120MHz Downlink
        Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (18100)); // 1930MHz Uplink

        Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (100)); // 20MHz UL Band
        Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (100)); // 20MHz DL Band
        
        lteHelper = CreateObject<LteHelper> ();

        lteEpcHelper = CreateObject<PointToPointEpcHelper> ();
        lteHelper->SetEpcHelper (lteEpcHelper);

        // Set propagation model
        //lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::ThreeGppUmaPropagationLossModel"));
        lteHelper->SetPathlossModelType (ns3::ThreeGppUmaPropagationLossModel::GetTypeId());
        lteHelper->SetPathlossModelAttribute ("ChannelConditionModel", PointerValue (new ns3::ThreeGppUmaChannelConditionModel()));
        lteHelper->SetPathlossModelAttribute ("ShadowingEnabled", BooleanValue (shadowing));
        // lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler"); // Round Robin scheduler

        pgw = lteEpcHelper->GetPgwNode ();
    }

    double startTime = 0.1;
    double endTime = startTime + simTime + 7;
    double totalSimTime = endTime + 11;

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
    NodeContainer ioMusTueNodes;
    NodeContainer ioMusTenbNodes;
    NodeContainer trafficUeNodes[numberOfTrafficEnbNodes];
    NodeContainer trafficEnbNodes;
    NodeContainer bandNodes[numberOfBands];
    ioMusTenbNodes.Create(numberOfenbNodes);
    enbNodes.Add(ioMusTenbNodes);
    if (traffic)
    {
        trafficEnbNodes.Create(numberOfTrafficEnbNodes);
        enbNodes.Add(trafficEnbNodes);
        for (int i = 0; i < numberOfTrafficEnbNodes; i++)
        {
            trafficUeNodes[i].Create(trafficUeNodesPerenb);
            ueNodes.Add(trafficUeNodes[i]);
        }
    }
    for(uint32_t u = 0; u < numberOfBands; u++)
    {
        bands[u] = combination.at(u);
        bandNodes[u].Create(bands[u]);
        ioMusTueNodes.Add(bandNodes[u]);
        ueNodes.Add(bandNodes[u]);
        std::cout << "Band " << u + 1 << ": " << bands[u] << std::endl;
    }

    std::cout << "Number of IoMusT UEs: " << ioMusTueNodes.GetN() << std::endl;
    std::cout << "Number of traffic UEs: " << ueNodes.GetN() - ioMusTueNodes.GetN() << std::endl;

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    std::vector<Vector> enbs;
    std::vector<Vector> trafficEnbs;
    int x_offset = multiplier*squareWidth/4;
    int y_offset = multiplier*433;
    int center = squareWidth/2;
    enbs.push_back(Vector(squareWidth/2, squareWidth/2, enbHeight));

    if (scenario == 2)
    {
        enbs.push_back(Vector(center + cell2cellDistance, center, enbHeight));
        enbs.push_back(Vector(center + x_offset, center + y_offset, enbHeight));
    }

    std::string positionsFileName = folder + "/data/" + fileNamePrefix + "_positions.txt";
    std::ofstream ofs;
    ofs.open(positionsFileName, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
    populate_positions(positionAlloc, numberOfueNodes, enbs, radius, height, positionsFileName, false);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(ioMusTueNodes);

    positionAlloc = CreateObject<ListPositionAllocator> ();
    for (Vector enb : enbs)
    {
        positionAlloc->Add (enb);
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(ioMusTenbNodes);

    if (traffic)
    {
        trafficEnbs.push_back(Vector(center - cell2cellDistance, center, enbHeight));
        trafficEnbs.push_back(Vector(center - x_offset, center + y_offset, enbHeight));
        trafficEnbs.push_back(Vector(center - x_offset, center - y_offset, enbHeight));
        trafficEnbs.push_back(Vector(center + x_offset, center - y_offset, enbHeight));
        
        if (scenario == 1)
        {
            trafficEnbs.push_back(Vector(center + cell2cellDistance, center, enbHeight));
            trafficEnbs.push_back(Vector(center + x_offset, center + y_offset, enbHeight));
        }
        else if (scenario == 2)
        {
            trafficEnbs.push_back(Vector(center + x_offset + cell2cellDistance, center - y_offset, enbHeight));
            trafficEnbs.push_back(Vector(center + 2*cell2cellDistance, center, enbHeight));
            trafficEnbs.push_back(Vector(center + x_offset + cell2cellDistance, center + y_offset, enbHeight));
            trafficEnbs.push_back(Vector(center + cell2cellDistance, center + y_offset + cell2cellDistance, enbHeight));
            trafficEnbs.push_back(Vector(center, center + y_offset + cell2cellDistance, enbHeight));
        }
        positionAlloc = CreateObject<ListPositionAllocator> ();
        for (Vector trafficEnb : trafficEnbs)
        {
            positionAlloc->Add(trafficEnb);
        }
        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(trafficEnbNodes);

        int i = 0;
        for (Vector trafficEnb: trafficEnbs)
        {
            positionAlloc = CreateObject<ListPositionAllocator> ();
            std::vector<Vector> trafficEnbVector;
            trafficEnbVector.push_back(trafficEnb);
            populate_positions(positionAlloc, trafficUeNodesPerenb,  trafficEnbVector, radius, height, positionsFileName, true);
            mobility.SetPositionAllocator(positionAlloc);
            mobility.Install(trafficUeNodes[i]);
            i++;
        }
    }
    
    positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (enbs.at(0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(remoteHost);
    mobility.Install(pgw);

    if (five_g)
    { // *** 5G ***
        // install nr net devices
        enbNetDevs = nrHelper->InstallGnbDevice(enbNodes, allBwps);
        ueNetDevs = nrHelper->InstallUeDevice (ueNodes, allBwps);

        int i;
        for(i = 0; i < numberOfenbNodes; i++) {
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
    }
    else
    { // *** 4G ***
        // Install LTE Devices to the nodes
        enbNetDevs = lteHelper->InstallEnbDevice (enbNodes);
        ueNetDevs = lteHelper->InstallUeDevice (ueNodes);
        
        int i;
        for(i = 0; i < numberOfenbNodes; i++) {
            Ptr<LteEnbNetDevice> dev = DynamicCast<LteEnbNetDevice> (enbNetDevs.Get(i));
            dev->GetRrc()->SetSrsPeriodicity(80);
        }
    }

    // Install the IP stack on the UEs
    internet.Install (ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    if (five_g) // *** 5G ***
        ueIpIface = nrEpcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDevs));
    else // *** 4G ***
        ueIpIface = lteEpcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDevs));

    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
        {
        Ptr<Node> ueNode = ueNodes.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        if (five_g) // *** 5G ***
            ueStaticRouting->SetDefaultRoute (nrEpcHelper->GetUeDefaultGatewayAddress (), 1);
        else // *** 4G ***
            ueStaticRouting->SetDefaultRoute (lteEpcHelper->GetUeDefaultGatewayAddress (), 1);
        }

    if (five_g) // *** 5G ***
        // attach UEs to the closest eNB
        nrHelper->AttachToClosestEnb (ueNetDevs, enbNetDevs);
    else // *** 4G ***
        // Attach one UE per eNodeB
        lteHelper->AttachToClosestEnb (ueNetDevs, enbNetDevs);
        // side effect: the default EPS bearer will be activated

    if (five_g)
    { // *** 5G ***
        ulTft = Create<EpcTft> ();
        EpcTft::PacketFilter ulpfLowLatency;
        ulpfLowLatency.direction = EpcTft::UPLINK;
        ulTft->Add(ulpfLowLatency);

        dlTft = Create<EpcTft> ();
        EpcTft::PacketFilter dlpfLowLatency;
        dlpfLowLatency.direction = EpcTft::DOWNLINK;
        dlTft->Add(dlpfLowLatency);
    }

    if (traffic)
    {
        UdpEchoServerHelper udpEchoServer(12);
        ApplicationContainer serverApps = udpEchoServer.Install (remoteHost);
        serverApps.Start (Seconds (startTime));
        serverApps.Stop (Seconds (endTime));

        for (int j = 0; j < numberOfTrafficEnbNodes; j++)
            for (uint32_t i = 0; i < trafficUeNodes[j].GetN(); i++)
            {
                UdpEchoClientHelper udpClient(remoteHostAddr, 12);
                udpClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
                udpClient.SetAttribute ("Interval", TimeValue (Seconds (0.001)));
                udpClient.SetAttribute ("PacketSize", UintegerValue (25));
                ApplicationContainer clientApps = udpClient.Install(trafficUeNodes[j].Get(i));
                clientApps.Start (Seconds ((rand() % 1000 + startTime*1000)/100.0));
                clientApps.Stop (Seconds (endTime));

                if(five_g)
                { // *** 5G ***
                    Ptr<Node> node = trafficUeNodes[j].Get(i);
                    Ptr<NetDevice> device = node->GetDevice(0);
                    nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerUL, ulTft);
                    nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerDL, dlTft);
                }
            }
    }

    int j;
    uint64_t *e2eLatVect[ioMusTueNodes.GetN()][2];
    for(uint32_t i = 0; i < ioMusTueNodes.GetN(); i++)
    {
        UdpIomustServerHelper udpServer(5);
        uint64_t *arrivalTimeVect = (uint64_t *) malloc(nPackets * sizeof(uint64_t));
        uint64_t *latVect = (uint64_t *) malloc(nPackets * sizeof(uint64_t));
        for(j = 0; j < nPackets; j++)
        {
            arrivalTimeVect[j] = 0;
            latVect[j] = 0;
        }
        e2eLatVect[i][0] = arrivalTimeVect;
        e2eLatVect[i][1] = latVect;
        ApplicationContainer ueServers = udpServer.Install(ioMusTueNodes.Get(i));
        Ptr<UdpIomustServer> server = udpServer.GetServer();
        server->SetMaxLatency(thrsLatency);
        server->SetDataVectors(arrivalTimeVect, latVect);
        server->SetNPackets(nPackets);
        ueServers.Start(Seconds(startTime));
        ueServers.Stop(Seconds(endTime));
    }

    for(uint16_t u = 0; u < numberOfBands; u++)
    {
        // Install and start applications on UEs and remote host
        UdpMixServerHelper udpMixServer(u + 50);
        udpMixServer.SetAttribute("TransmissionPeriod", TimeValue(Seconds(interPacketInterval)));
        udpMixServer.SetAttribute("Timeout", TimeValue(MilliSeconds(mixServerTimeout)));
        ApplicationContainer serverApps = udpMixServer.Install (remoteHost);
        serverApps.Start (Seconds (startTime));
        serverApps.Stop (Seconds (endTime));

        for(uint16_t i = 0; i < bands[u]; i++)
        {
            UdpIomustClientHelper udpClient(remoteHostAddr, 50 + u);
            udpClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
            udpClient.SetAttribute ("Interval", TimeValue (Seconds (interPacketInterval)));
            ApplicationContainer clientApps = udpClient.Install(bandNodes[u].Get(i));
            uint8_t *data = new uint8_t[5];
            udpClient.SetFill(clientApps.Get(0), data, 5, packetSize);
            double sTime = (rand() % 500 + startTime*1000)/1000.0;
            clientApps.Start (Seconds (sTime));
            clientApps.Stop (Seconds (endTime));

            if(five_g)
            { // *** 5G ***
                Ptr<Node> node = bandNodes[u].Get(i);
                Ptr<NetDevice> device = node->GetDevice(0);
                nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerUL, ulTft);
                nrHelper->ActivateDedicatedEpsBearer (device, lowLatencyBearerDL, dlTft);
            }
        }
    }

    // if (five_g) // *** 5G ***
    //     nrHelper->EnableTraces();
    // else // *** 4G ***
    //     lteHelper->EnableTraces ();

    // Flow monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.Install(ueNodes);
    flowHelper.Install(remoteHost);

    // AnimationInterface anim ("animation.xml");
    // anim.SetMaxPktsPerTraceFile(5000000);
    // anim.SetConstantPosition(remoteHost, squareWidth, squareWidth);

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
        for(int j = 0; j < nPackets; j++) {
        timeStamp = (uint64_t) e2eLatVect[i][0][j];
        val = (uint64_t) e2eLatVect[i][1][j];
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

    int i;
    for(i = 0; i < numberOfueNodes; i++)  {
        free(e2eLatVect[i][0]);
        free(e2eLatVect[i][1]);
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