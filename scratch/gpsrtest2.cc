#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/gpsr-module.h"
#include "ns3/stats-module.h"


#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");

using namespace ns3;

int packetsReceived = 0;

void ReceivePacket (Ptr<Socket> socket)
{
 
  NS_LOG_UNCOND ("Received One packet!");
  uint32_t bytesTotal = 0;
   Ptr<Packet> packet;
   while(packet = socket->Recv())
    {
        bytesTotal += packet->GetSize ();
              packetsReceived += 1;
        }
  
}
static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }

}

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  uint32_t packetSize = 1024; // bytes
  uint32_t numPackets = 50;
  uint32_t nNodes = 5;
  double interval = 2.0; // seconds

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue("nNodes", "Number of Nodes", nNodes);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

 


  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("22000"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (nNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  //wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
 // wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  //wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  //wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel","MinDistance",DoubleValue (100));
 
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
        "Exponent", DoubleValue (2.5) ,
        "ReferenceDistance" , DoubleValue(1.0) ,
        "ReferenceLoss"    ,DoubleValue(37.35));

  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  Ns2MobilityHelper ns2 = Ns2MobilityHelper ("/home/hry-user/ns-allinone-3.26/ns-3.26/node/test.tcl");
  ns2.Install ();
 
//  AodvHelper aodv;
  // OlsrHelper olsr;
  GpsrHelper gpsr;
 
  //aodv.Install();

  InternetStackHelper internet;
  internet.SetRoutingHelper (gpsr);
  internet.Install (c);
  gpsr.Install ();

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  
  uint16_t Port = 12345;
  
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
 
 
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local = InetSocketAddress ((Ipv4Address::GetAny())/*("10.1.1.4")*/, Port);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
 
  Ptr<Socket> source;
  source = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress remote = InetSocketAddress (("10.1.1.4"), Port);
  //source->SetAllowBroadcast (true);
  source->Connect (remote);
 
   //Tracing
//   AsciiTraceHelper ascii;
  //wifiPhy.EnableAsciiAll (ascii.CreateFileStream("scratch/30nodes.tr"));//, devices);
    //wifiPhy.EnablePcap("scratch/30nodes",devices);


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.Install(c);

        // Output what we are doing
  NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent " );
 
 
  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (1.0), &GenerateTraffic, source, packetSize, numPackets, interPacketInterval);
  Simulator::Stop(Seconds (120));
  Simulator::Run ();

 

  monitor->CheckForLostPackets ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
 
          std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";

 
            std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024  << " Kbps\n";
          std::cout << "transmitted packets: " <<i->second.txPackets << "\n";     
          std::cout << "recived packets: " <<i->second.rxPackets << "\n";
          std::cout<< " Lost Packets: " << ((i->second.txBytes) - (i->second.rxBytes))/ packetSize <<"\n";
       //   std::cout << "  Delay Sum:   " << i->second.delaySum / 1000000 << "ms\n";
     }


  //monitor->SerializeToXmlFile("scratch/new30_1.flowmon", true, true);

  Simulator::Destroy ();
  std::cout << "packets received are " << packetsReceived << "\n";

  return 0;
}