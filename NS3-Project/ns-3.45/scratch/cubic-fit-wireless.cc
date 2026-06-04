/*

 * Network Topology:
 *
 *   Wifi 192.168.20.0
 *                   AP				       AP
 *  *    *    *  *   *
 *  |    |    |  |   |    192.168.10.0
 * n2   n3   n4  n5  n0 -------------- n1   n6   n7   n8  n9
 *                   point-to-point    |    |    |    |   |
 *                                     *    *    *    *   *
 *                                       Wifi 192.168.30.0
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/energy-module.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/wifi-radio-energy-model-helper.h"

using namespace ns3;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("CubicFitWireless");

// simple packet sink application
class MyApp : public Application
{
  public:
    MyApp();
    virtual ~MyApp();
    static TypeId GetTypeId(void);
    void Setup(Ptr<Socket> socket,
               Address address,
               uint32_t packetSize,
               uint32_t nPackets,
               DataRate dataRate);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

MyApp::~MyApp()
{
    m_socket = 0;
}

TypeId
MyApp::GetTypeId(void)
{
    static TypeId tid =
        TypeId("MyApp").SetParent<Application>().SetGroupName("Tutorial").AddConstructor<MyApp>();
    return tid;
}

void
MyApp::Setup(Ptr<Socket> socket,
             Address address,
             uint32_t packetSize,
             uint32_t nPackets,
             DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void
MyApp::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        m_socket->Bind();
    }
    else
    {
        m_socket->Bind6();
    }
    m_socket->Connect(m_peer);
    SendPacket();
}

void
MyApp::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void
MyApp::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx();
    }
}

void
MyApp::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
    }
}

// global trace files
std::ofstream alphaFile;
std::ofstream nFlowsFile;
std::ofstream queueFile;
std::ofstream cwndFile;

// trace callbacks
void
AlphaTracer(double oldVal, double newVal)
{
    alphaFile << Simulator::Now().GetSeconds() << " " << newVal << std::endl;
}

void
NTracer(double oldVal, double newVal)
{
    nFlowsFile << Simulator::Now().GetSeconds() << " " << newVal << std::endl;
}

void
QueueTracer(Time oldVal, Time newVal)
{
    queueFile << Simulator::Now().GetSeconds() << " " << newVal.GetSeconds() << std::endl;
}

void
CwndTracer(uint32_t oldVal, uint32_t newVal)
{
    cwndFile << Simulator::Now().GetSeconds() << " " << newVal << std::endl;
}

// void
// PhyRxErrorTrace(std::string context, Ptr<const Packet> packet, double snr)
// {
//     // Implementation ... if needed
// }

int
main(int argc, char* argv[])
{
    LogComponentEnable("TcpCubicFit", LOG_LEVEL_INFO);
    uint32_t payloadSize = 1472;
    std::string tcpCongestionAlgo = "ns3::TcpCubic";
    std::string outputFilename = "wirelesshigh_cubic_nodes.txt";
    std::string tracePrefix = ""; // empty implies no tracing

    double simulationTime = 10.0; // seconds
    uint32_t no_of_TCP_flows = 2; // flows per side
    uint32_t nWifiNodes = 6;      // nodes per side changed from 5 to 6 per user request

    double range = 100.0;     // range for propagation loss
    double errorRate = 0.001; // packet error rate

    uint32_t packetSize = 1024;
    uint32_t nPackets = 10000;
    std::string dataRate = "10Mbps"; // defaults if not parsing pps
    uint16_t sinkPort = 8080;
    std::string p2pDelay = "20ms";

    uint32_t pps = 0; // packets per second (overrides dataRate if > 0)
    double nodeSpeed = 2.0; // mobility speed (m/s)
    uint32_t coverageScale = 1; // multiplier for 50x50 coverage area
    bool isMobile = true; 

    CommandLine cmd(__FILE__);
    cmd.AddValue("nFlows", "Number of TCP Flows", no_of_TCP_flows);
    cmd.AddValue("nWifi", "Number of Wifi Nodes per side", nWifiNodes);
    cmd.AddValue("errorRate", "Packet Error Rate", errorRate);
    cmd.AddValue("p2pDelay", "P2P Link Delay", p2pDelay);
    cmd.AddValue("range", "Transmission Range", range);
    cmd.AddValue("transport_prot", "Transport Protocol", tcpCongestionAlgo);
    cmd.AddValue("output_file", "Output Filename", outputFilename);
    cmd.AddValue("trace_prefix", "Prefix for trace files (empty to disable)", tracePrefix);
    
    // new cubic arguments
    cmd.AddValue("pps", "Packets Per Second (Overrides app rate)", pps);
    cmd.AddValue("nodeSpeed", "Speed of nodes (m/s) for mobile scenario", nodeSpeed);
    cmd.AddValue("coverageScale", "Scale of Coverage Area (1x to 5x of fixed range)", coverageScale);
    cmd.AddValue("isMobile", "Enable Node Mobility", isMobile);
    cmd.Parse(argc, argv);

    if (pps > 0) {
        // app dataRate = pps * packetSize * 8 bits
        uint64_t bps = pps * packetSize * 8ULL;
        dataRate = std::to_string(bps) + "bps";
    }

    // setting default tcp variant
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(tcpCongestionAlgo)));

    // opening trace files if requested
    if (!tracePrefix.empty())
    {
        alphaFile.open((tracePrefix + "_alpha.dat").c_str());
        nFlowsFile.open((tracePrefix + "_n.dat").c_str());
        queueFile.open((tracePrefix + "_queue.dat").c_str());
        cwndFile.open((tracePrefix + "_cwnd.dat").c_str());

        // connecting traces
        // cwnd is common to all
        Config::ConnectWithoutContext(
            "/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",
            MakeCallback(&CwndTracer));

        // CUBIC-FIT specific traces
        if (tcpCongestionAlgo == "ns3::TcpCubicFit")
        {
            Config::ConnectWithoutContext("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/"
                                          "CongestionOps/$ns3::TcpCubicFit/Alpha",
                                          MakeCallback(&AlphaTracer));
            Config::ConnectWithoutContext("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/"
                                          "CongestionOps/$ns3::TcpCubicFit/NFlows",
                                          MakeCallback(&NTracer));
            Config::ConnectWithoutContext("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/"
                                          "CongestionOps/$ns3::TcpCubicFit/QueueDelay",
                                          MakeCallback(&QueueTracer));
        }
    }

    // topology construction
    // p2p backbone: n0 -- n1
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue(p2pDelay));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    // wifi left (connected to n0)
    NodeContainer wifiStaNodesLeft;
    wifiStaNodesLeft.Create(nWifiNodes);
    NodeContainer wifiApNodeLeft = p2pNodes.Get(0);

    // wifi right (connected to n1)
    NodeContainer wifiStaNodesRight;
    wifiStaNodesRight.Create(nWifiNodes);
    NodeContainer wifiApNodeRight = p2pNodes.Get(1);

    // wifi phy/channel setup
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(range));

    YansWifiPhyHelper phyLeft;
    phyLeft.SetChannel(channel.Create());
    // error model for left
    Ptr<RateErrorModel> emLeft = CreateObject<RateErrorModel>();
    emLeft->SetAttribute("ErrorRate", DoubleValue(errorRate));
    phyLeft.SetErrorRateModel("ns3::YansErrorRateModel"); // using standard model + PostReception?
    // note: previous code set PostReceptionErrorModel manually. Let's do that cleanly.

    YansWifiPhyHelper phyRight;
    phyRight.SetChannel(channel.Create());
    // error model for right
    Ptr<RateErrorModel> emRight = CreateObject<RateErrorModel>();
    emRight->SetAttribute("ErrorRate", DoubleValue(errorRate));
    phyRight.SetErrorRateModel("ns3::YansErrorRateModel");

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("CubicFit-Net"); // changed SSID

    // installing wifi left
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevicesLeft = wifi.Install(phyLeft, mac, wifiStaNodesLeft);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevicesLeft = wifi.Install(phyLeft, mac, wifiApNodeLeft);

    // installing wifi right
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevicesRight = wifi.Install(phyRight, mac, wifiStaNodesRight);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevicesRight = wifi.Install(phyRight, mac, wifiApNodeRight);

    // applying error models
    // iterating over sta devices and applying error model
    for (uint32_t i = 0; i < staDevicesLeft.GetN(); ++i)
    {
        PointerValue ptr;
        staDevicesLeft.Get(i)->GetAttribute("Phy", ptr);
        Ptr<WifiPhy> phy = ptr.Get<WifiPhy>();
        phy->SetPostReceptionErrorModel(emLeft);
    }
    for (uint32_t i = 0; i < staDevicesRight.GetN(); ++i)
    {
        PointerValue ptr;
        staDevicesRight.Get(i)->GetAttribute("Phy", ptr);
        Ptr<WifiPhy> phy = ptr.Get<WifiPhy>();
        phy->SetPostReceptionErrorModel(emRight);
    }

    // mobility
    // calculating coverage area based on scale (base 50x50m)
    double areaSide = 50.0 * coverageScale;
    
    MobilityHelper mobilitySta;
    if (isMobile) {
        // 1. mobile client stations
        mobilitySta.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                          "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(areaSide) + "]"),
                                          "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(areaSide) + "]"));
                                          
        mobilitySta.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                     "Bounds", RectangleValue(Rectangle(0.0, areaSide, 0.0, areaSide)),
                                     "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(nodeSpeed) + "]")); 
    } else {
        // 1. static client stations configured dynamically
        mobilitySta.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0),
                                      "DeltaX", DoubleValue(5.0 * coverageScale), // expand spacing linearly
                                      "DeltaY", DoubleValue(5.0 * coverageScale),
                                      "GridWidth", UintegerValue(3),
                                      "LayoutType", StringValue("RowFirst"));
        mobilitySta.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    }
    mobilitySta.Install(wifiStaNodesLeft);
    mobilitySta.Install(wifiStaNodesRight);

    // 2. fixed access points (placed in the center of the area)
    MobilityHelper mobilityAp;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(areaSide / 2.0, areaSide / 2.0, 0.0));
    mobilityAp.SetPositionAllocator(positionAlloc);
    mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    
    mobilityAp.Install(wifiApNodeLeft);
    mobilityAp.Install(wifiApNodeRight);
    
    // 3. energy module installation
    BasicEnergySourceHelper basicSourceHelper;
    basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(1000.0));
    
    EnergySourceContainer sourcesLeft = basicSourceHelper.Install(wifiStaNodesLeft);
    EnergySourceContainer sourcesRight = basicSourceHelper.Install(wifiStaNodesRight);
    EnergySourceContainer sourcesApLeft = basicSourceHelper.Install(wifiApNodeLeft);
    EnergySourceContainer sourcesApRight = basicSourceHelper.Install(wifiApNodeRight);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    DeviceEnergyModelContainer deviceModels;
    deviceModels.Add(radioEnergyHelper.Install(staDevicesLeft, sourcesLeft));
    deviceModels.Add(radioEnergyHelper.Install(staDevicesRight, sourcesRight));
    deviceModels.Add(radioEnergyHelper.Install(apDevicesLeft, sourcesApLeft));
    deviceModels.Add(radioEnergyHelper.Install(apDevicesRight, sourcesApRight));

    EnergySourceContainer sources;
    sources.Add(sourcesLeft);
    sources.Add(sourcesRight);
    sources.Add(sourcesApLeft);
    sources.Add(sourcesApRight);

    // installing internet stack
    InternetStackHelper stack;
    stack.Install(p2pNodes);
    stack.Install(wifiStaNodesLeft);
    stack.Install(wifiStaNodesRight);

    // ip assignment
    Ipv4AddressHelper address;

    address.SetBase("192.168.10.0", "255.255.255.0"); 
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("192.168.20.0", "255.255.255.0"); 
    address.Assign(staDevicesLeft);
    address.Assign(apDevicesLeft);

    address.SetBase("192.168.30.0", "255.255.255.0"); 
    Ipv4InterfaceContainer staInterfacesRight = address.Assign(staDevicesRight);
    address.Assign(apDevicesRight);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // installing applications
    // traffic from left STAs to right STAs
    for (uint32_t i = 0; i < no_of_TCP_flows && i < nWifiNodes; ++i)
    {
        Address sinkAddress(InetSocketAddress(staInterfacesRight.GetAddress(i), sinkPort));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
        ApplicationContainer sinkApps = sinkHelper.Install(wifiStaNodesRight.Get(i));
        sinkApps.Start(Seconds(0.));
        sinkApps.Stop(Seconds(simulationTime));

        Ptr<Socket> ns3TcpSocket =
            Socket::CreateSocket(wifiStaNodesLeft.Get(i), TcpSocketFactory::GetTypeId());
        Ptr<MyApp> app = CreateObject<MyApp>();
        app->Setup(ns3TcpSocket, sinkAddress, packetSize, nPackets, DataRate(dataRate));
        wifiStaNodesLeft.Get(i)->AddApplication(app);
        app->SetStartTime(Seconds(1.0 + i * 0.1)); // staggered start
        app->SetStopTime(Seconds(simulationTime));
    }

    // CWND tracing for plan 1 & 3 analysis
    AsciiTraceHelper asciiTraceHelper;
 
  
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    std::cout << "Starting Simulation..." << std::endl;
    Simulator::Stop(Seconds(simulationTime + 1.0));
    Simulator::Run();

 
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double totalThroughput = 0;
    double jainsIndexNum = 0;
    double jainsIndexDenom = 0;
    double totalDelay = 0;
    uint32_t totalRx = 0;
    uint32_t totalTx = 0;
    uint32_t flowCount = 0;

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

             if (t.destinationPort == sinkPort)
        {
            double throughput = i->second.rxBytes * 8.0 / (simulationTime * 1000000.0); // Mbps
            totalThroughput += throughput;
            jainsIndexNum += throughput;
            jainsIndexDenom += throughput * throughput;
            totalDelay += i->second.delaySum.GetSeconds();
            totalRx += i->second.rxPackets;
            totalTx += i->second.txPackets;
            flowCount++;

            std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                      << t.destinationAddress << ")\n";
            std::cout << "  Throughput: " << throughput << " Mbps\n";
            std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
            std::cout << "  Per-Flow Delay: "
                      << i->second.delaySum.GetSeconds() / (i->second.rxPackets + 1) << " s\n";
        }
    }

    double jainsIndex =
        (flowCount > 0) ? (jainsIndexNum * jainsIndexNum) / (flowCount * jainsIndexDenom) : 0;
    double avgDelay = (totalRx > 0) ? totalDelay / totalRx : 0; // seconds
    double pdr = (totalTx > 0) ? (double)totalRx / totalTx * 100.0 : 0;
    double dropRatio = (totalTx > 0) ? (double)(totalTx - totalRx) / totalTx * 100.0 : 0;

    std::cout << "\n=== Summary Results ===\n";
    std::cout << "Total Throughput: " << totalThroughput << " Mbps\n";
    std::cout << "Avg End-to-End Delay: " << avgDelay * 1e9
              << " ns\n"; // output in ns for plot consistency
    std::cout << "Packet Delivery Ratio: " << pdr << " %\n";
    std::cout << "Packet Drop Ratio: " << dropRatio << " %\n";
    std::cout << "Jain's Fairness Index: " << jainsIndex << "\n";

    // calculating energy
    double totalEnergyConsumed = 0.0;
    for (auto src = sources.Begin(); src != sources.End(); ++src) {
        double currentEnergy = (*src)->GetRemainingEnergy();
        totalEnergyConsumed += (1000.0 - currentEnergy);
    }
    std::cout << "Total Energy Consumed: " << totalEnergyConsumed << " Joules\n";
    std::cout << "=======================\n";

    
    double throughputKbps = totalThroughput * 1000.0;
    double delayNs = avgDelay * 1e9;

    std::ofstream outFile;
    std::string filename = outputFilename; 

    outFile.open(filename, std::ios_base::app);
    if (outFile.is_open())
    {
        outFile << (nWifiNodes * 2) << " " // Total nodes (Nodes * 2 sides)
                << throughputKbps << " "   // Throughput Kbps
                << "ns "                   // Unit dummy (name)
                << delayNs << " "          // Delay ns
                << pdr << " "              // PDR
                << dropRatio << " "        // Drop
                << totalEnergyConsumed << "\n"; // Energy
        outFile.close();
    }

    Simulator::Destroy();
    return 0;
}
