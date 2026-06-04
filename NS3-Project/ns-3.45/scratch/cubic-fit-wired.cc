
/*
 * Network Topology (Dynamic Point-to-Point Dumbbell):
   Left Network (192.168.1.0)                   Right Network (192.168.2.0)
   n2   n3   n4   n5  ...                    ...  n6   n7   n8   n9
    |    |    |    |                               |    |    |    |
     \   |    |   /    Bottleneck 192.168.3.0       \   |    |   /
      \  |    |  /        (10 Mbps, 100 ms)          \  |    |  /
        Router 0  ==================================  Router 1
 * Link = Bottleneck (Configure via --bw, --p2pDelay, --errorRate)
 * Sn/Dn = Scaled dynamically via --nNodes
 */
 
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CubicFitWired");

int main(int argc, char* argv[])
{
    std::string bottleneckBw = "10Mbps";
    std::string p2pDelay = "100ms";
    double errorRate = 0.0;
    
    uint32_t nWifi = 10;
    uint32_t nFlows = 10;
    uint32_t pps = 100;

    double simTime = 100.0;
    std::string transport_prot = "ns3::TcpCubicFit";

    CommandLine cmd(__FILE__);
    cmd.AddValue("bw", "Bottleneck Bandwidth", bottleneckBw);
    cmd.AddValue("p2pDelay", "Bottleneck P2P Delay", p2pDelay);
    cmd.AddValue("errorRate", "Packet Error Rate", errorRate);
    cmd.AddValue("transport_prot", "TCP Protocol", transport_prot);
    cmd.AddValue("nWifi", "Number of Dumbbell Leaves (reused arg name)", nWifi);
    cmd.AddValue("nFlows", "Number of TCP Flows", nFlows);
    cmd.AddValue("pps", "Packets Per Second", pps);
    cmd.Parse(argc, argv);

    // calculating data rate string based on pps (packet size = 1000 bytes)
    uint32_t dataRateBps = pps * 1000 * 8;
    std::string dataRateStr = std::to_string(dataRateBps) + "bps";

    uint32_t nTotal = nWifi;
    if (nTotal < 1) nTotal = 1;

    DataRate bwRate(bottleneckBw);
    Time delayTime(p2pDelay);
    uint32_t bdp_bytes = bwRate.GetBitRate() / 8.0 * (delayTime.GetSeconds() * 2);
    uint32_t queueSize = std::max<uint32_t>(5, (bdp_bytes / 1000) / 2);
    std::string queueSizeStr = std::to_string(queueSize) + "p";

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));

    PointToPointHelper p2pAccess;
    p2pAccess.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2pAccess.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointHelper p2pBottleneck;
    p2pBottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckBw));
    p2pBottleneck.SetChannelAttribute("Delay", StringValue(p2pDelay));
    p2pBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(queueSizeStr));

    if (errorRate > 0)
    {
        Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
        em->SetAttribute("ErrorRate", DoubleValue(errorRate));
        p2pBottleneck.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));
    }

    PointToPointDumbbellHelper dumbbell(nTotal, p2pAccess, nTotal, p2pAccess, p2pBottleneck);

    InternetStackHelper stack;
    dumbbell.InstallStack(stack);

    dumbbell.AssignIpv4Addresses(Ipv4AddressHelper("192.168.1.0", "255.255.255.0"),
                                 Ipv4AddressHelper("192.168.2.0", "255.255.255.0"),
                                 Ipv4AddressHelper("192.168.3.0", "255.255.255.0"));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //for left nodes
    for (uint32_t i = 0; i < nTotal; i++)
    {
        Ptr<Node> node = dumbbell.GetLeft(i);
        Config::Set("/NodeList/" + std::to_string(node->GetId()) + "/$ns3::TcpL4Protocol/SocketType",
                    TypeIdValue(TypeId::LookupByName(transport_prot)));
    }

    
    uint16_t port = 50000;
    for (uint32_t i = 0; i < nFlows; i++)
    {
        uint32_t nodeIndex = i % nTotal;
        
        InetSocketAddress sinkAddress(dumbbell.GetRightIpv4Address(nodeIndex), port + i);
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), port + i));
        ApplicationContainer sinkApp = sinkHelper.Install(dumbbell.GetRight(nodeIndex));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime + 1.0));

        OnOffHelper sourceHelper("ns3::TcpSocketFactory", sinkAddress);
        sourceHelper.SetAttribute("DataRate", StringValue(dataRateStr));
        sourceHelper.SetAttribute("PacketSize", UintegerValue(1000));
        sourceHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        sourceHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        
        ApplicationContainer sourceApp = sourceHelper.Install(dumbbell.GetLeft(nodeIndex));
        sourceApp.Start(Seconds(0.1));
        sourceApp.Stop(Seconds(simTime));
    }

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    double totalDelay = 0;
    uint32_t totalRx = 0;
    uint32_t totalTx = 0;
    double jainsIndexNum = 0;
    double jainsIndexDenom = 0;
    uint32_t flowCount = 0;

    for (const auto& stat : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(stat.first);
        if (t.protocol == 6) // selects only TCP
        {
            double throughput = stat.second.rxBytes * 8.0 / (simTime * 1000000.0); // Mbps
            jainsIndexNum += throughput;
            jainsIndexDenom += throughput * throughput;
            
            totalDelay += stat.second.delaySum.GetSeconds();
            totalRx += stat.second.rxPackets;
            totalTx += stat.second.txPackets;
            flowCount++;
        }
    }

    double totalThroughput = (flowCount > 0) ? jainsIndexNum : 0;
    double jainsIndex = (flowCount > 0 && jainsIndexDenom > 0) ? (jainsIndexNum * jainsIndexNum) / (flowCount * jainsIndexDenom) : 0;
    double avgDelay = (totalRx > 0) ? (totalDelay / totalRx) : 0; // seconds
    double pdr = (totalTx > 0) ? (double)totalRx / totalTx * 100.0 : 0;
    double dropRatio = (totalTx > 0) ? (double)(totalTx - totalRx) / totalTx * 100.0 : 0;

    std::cout << "\n=== Summary Results ===\n";
    std::cout << "Total Throughput: " << totalThroughput << " Mbps\n";
    std::cout << "Avg End-to-End Delay: " << avgDelay * 1e9 << " ns\n";
    std::cout << "Packet Delivery Ratio: " << pdr << " %\n";
    std::cout << "Packet Drop Ratio: " << dropRatio << " %\n";
    std::cout << "Jain's Fairness Index: " << jainsIndex << "\n";
    std::cout << "Total Energy Consumed: 0 Joules\n";
    std::cout << "=======================\n";

    Simulator::Destroy();
    return 0;
}
