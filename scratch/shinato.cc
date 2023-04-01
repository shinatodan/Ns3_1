#include <fstream>
#include <iostream>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/gpsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/integer.h"
#include "ns3/wave-bsm-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ns2-mobility-helper.h"
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("shinato-simulation");

//文字列を送信し受信したら表示するプログラム
//GPSRでは使用不可

class WifiPhyStats : public Object
{
public:
  static TypeId GetTypeId (void);
  WifiPhyStats ();
  virtual ~WifiPhyStats ();
  void PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower);
  uint64_t GetPhyTxBytes ();
private:
  uint64_t m_phyTxBytes;
};
NS_OBJECT_ENSURE_REGISTERED (WifiPhyStats);
TypeId
WifiPhyStats::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhyStats")
  .SetParent<Object> ()
  .AddConstructor<WifiPhyStats> ();
  return tid;
}
WifiPhyStats::WifiPhyStats ()
:m_phyTxBytes (0)
{
}
WifiPhyStats::~WifiPhyStats ()
{
}
void
WifiPhyStats::PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{
  uint64_t pktSize = packet->GetSize ();
  m_phyTxBytes += pktSize;
}
uint64_t
WifiPhyStats::GetPhyTxBytes ()
{
  return m_phyTxBytes;
}


class VanetRoutingExperiment
{
public:
    VanetRoutingExperiment(); //コンストラクタ　パラメータの初期化

    void Simulate (int argc, char **argv);
    virtual void SetDefaultAttributeValues ();//デフォルトの属性値を設定する
    virtual void ParseCommandLineArguments (int argc, char **argv);//コマンドライン引数を処理する//argc:引数の数,argv:引数
    virtual void ConfigureNodes ();//ノードを構成する
    virtual void ConfigureDevices ();//チャネルを構成する
    virtual void ConfigureMobility ();//モビリティを設定する
    virtual void ConfigureNetwork ();//ルーティングプロトコル,IPアドレス設定
    virtual void ConfigureApplications ();//アプリケーションを設定する
    virtual void RunSimulation ();//シミュレーションを実行する
    void ConfigureDefaults ();//デフォルトの属性を設定する
    void RunFlowMonitor();
  
    static void
    CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility);

    void ReceivePacket (Ptr<Socket> socket);
    void SendPacket (Ptr<Socket> socket, Time pktInterval);

private:

    uint32_t Port;
    uint32_t sumNode;
    uint32_t numPacket;
    uint32_t sourceNode[1];
    uint32_t sinkNode[1];
    uint32_t sendPacket;
    uint32_t receivePacket;
    std::string protocolName;

    double totalSimTime;

    double txp;
    double EDT;
    std::string lossModelName;
    std::string rate;
    std::string phyMode;
    uint32_t packetSize;

    uint32_t areaSize;

    NetDeviceContainer adhocDevice;
    Ipv4InterfaceContainer adhocInterfaces;
    NodeContainer adhocNodes;
    NodeContainer stopDevice;

    FlowMonitorHelper *flowMonitorHelper;
    Ptr<FlowMonitor>   flowMonitor;

    Ptr<WifiPhyStats> m_wifiPhyStats;

    //出力値
    double m_pdr;
    double m_throughput;
    double m_delay;
    double m_overHead;
    double m_packetLoss;
    double m_numHops;

    std::string traceFile;
};

VanetRoutingExperiment::VanetRoutingExperiment()
: Port (9),//ポート番号
sumNode (5),//ノード数
numPacket (50),
sendPacket (0),
receivePacket (0),
protocolName ("AODV"),//プロトコル名初期化
totalSimTime (360.0),// シミュレーション時間
txp (16.026),//送信電力(dB)
EDT (-96),
lossModelName ("ns3::LogDistancePropagationLossModel"),//電波伝搬損失モデルの名前
rate ("8192bps"),//レートを"8192bps"で初期化
phyMode ("OfdmRate24MbpsBW10MHz"),//wifiの物理層のモードを変調方式ofdm,レート6Mbps,帯域幅10MHz
packetSize(1024),//パケットサイズ
areaSize (600),
adhocNodes (),//ノードを初期化
m_pdr (0),
m_throughput (0),
m_delay (0),
m_overHead (0),
m_packetLoss (0),
m_numHops(0),
traceFile("/home/hry-user/ns-allinone-3.26/ns-3.26/node/test.tcl")//モビリティトレースファイル
{
    //送受信ノード選択
    sourceNode[0] = 0;
    sinkNode[0] = 3;

    m_wifiPhyStats = CreateObject<WifiPhyStats> ();
}

void
VanetRoutingExperiment::Simulate (int argc, char **argv)
{
    SetDefaultAttributeValues ();//デフォルトの属性値を設定する
    ParseCommandLineArguments (argc, argv);//コマンドライン引数を処理する argc:引数の数,argv:引数
    ConfigureNodes ();//ノードを構成する
    ConfigureDevices ();//チャネルを構成する
    ConfigureMobility ();//モビリティーを設定する
    ConfigureNetwork ();//ルーティング・プロトコル、IPアドレス設定
    ConfigureApplications ();//アプリケーション設定
    RunSimulation ();//シミュレーションを実行する
}

void
VanetRoutingExperiment::SetDefaultAttributeValues ()//デフォルトの属性値を設定する
{
}

void
VanetRoutingExperiment::ParseCommandLineArguments (int argc, char **argv)
{//コマンドライン引数を処理する　argc:引数の数,argv:引数

  CommandLine cmd;//コマンドライン引数を解析する

  //AddValue(プログラム提供の引数の名前,--PrintHelpで使用されるヘルプテキスト,値が格納される変数への参照)
  // コマンドライン引数で以下の変数を上書きする
  cmd.AddValue ("protocolName", "name of protocol", protocolName);
  cmd.AddValue ("simTime", "total simulation time", totalSimTime);
  cmd.Parse (argc, argv);
  //プログラムの引数を解析する。argc:引数の数(最初の要素としてメインプログラムの名前を含む),argv:nullで終わる文字列の配列,それぞれがコマンドライン引数を識別する
  ConfigureDefaults();
}

void
VanetRoutingExperiment::ConfigureDefaults ()//デフォルトの属性を設定する
{
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));
  //ユニキャスト以外の送信に使用されるwifiモードを変調方式ofdm,レート6Mbps,帯域幅10MHzとする

  //test
  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("22000"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
}

void
VanetRoutingExperiment::ConfigureNodes ()//ノードを作成する
{
  adhocNodes.Create (sumNode-1);
  stopDevice.Create(1);
  adhocNodes.Add(stopDevice);
}

void
VanetRoutingExperiment::ConfigureDevices ()//チャネルを構成する
{

    // Setup propagation models
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    //このチャネルの電波伝搬遅延モデルを設定する(電波伝搬速度は一定の速度2.99792e+08 )
    wifiChannel.AddPropagationLoss (lossModelName,
        "Exponent", DoubleValue (2.5) ,
        "ReferenceDistance" , DoubleValue(1.0) ,
        "ReferenceLoss"    ,DoubleValue(37.35));
    //電波伝搬損失モデル(二波モデル)を追加で設定し、その周波数とアンテナの高さの属性を設定する
    Ptr<YansWifiChannel> channel = wifiChannel.Create ();//以前に設定したパラメータに基づいてチャネルを作成する


    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();//デフォルトの動作状態でphyヘルパーを作成する
    wifiPhy.SetChannel (channel);//このヘルパーにチャネルを関連付ける
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
    //pcapトレースのデータリンクタイプをieee802.11無線LANヘッダーで設定する

    // Setup WAVE PHY and MAC
    NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();//デフォルトの動作状態でmacヘルパーを作成する
    Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();//デフォルト状態の新しいwifi80211phelperを返す

    // Setup 802.11p stuff
    wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",//データとrts送信に一定のレートを使用する
    "DataMode",StringValue (phyMode),//すべてのデータパケットの送信モードをOfdmRate6MbpsBW10MHzとする
    "ControlMode",StringValue (phyMode));//すべてのrtsパケットの送信モードをOfdmRate6MbpsBW10MHzとする

    // Set Tx Power
    wifiPhy.Set ("TxPowerStart",DoubleValue (txp));//最小送信レベルを20dbmとする
    wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));//最大送信レベルを20dbmとする
    wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (EDT));

    // Add an upper mac and disable rate control
    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac");//MAC層(AdhocWifiMac)を作成する

    // Setup net devices
    adhocDevice = wifi80211p.Install (wifiPhy, wifi80211pMac, adhocNodes);//ノードにネットデバイスを作成する
    Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback (&WifiPhyStats::PhyTxTrace, m_wifiPhyStats));
  
  }

void
VanetRoutingExperiment::ConfigureMobility ()
{//モビリティを設定する

   // Create Ns2MobilityHelper with the specified trace log file as parameter
      Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
      
      ns2.Install (adhocNodes.Begin (),adhocNodes.End());
      
      WaveBsmHelper::GetNodesMoving ().resize (48, 0);
 
}
void
VanetRoutingExperiment:://トレースファイルを読みだすのに必要
CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity

  pos.z = 1.5;

  int nodeId = mobility->GetObject<Node> ()->GetId ();
  double t = (Simulator::Now ()).GetSeconds ();
  if (t >= 1.0)
    {
      WaveBsmHelper::GetNodesMoving ()[nodeId] = 1;
    }
  // Prints position and velocities
  *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
      << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
      << ", z=" << vel.z << std::endl;
}

void 
VanetRoutingExperiment::ConfigureNetwork ()
{
    AodvHelper aodv;
    OlsrHelper olsr;
    GpsrHelper gpsr;

    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;
    Ipv4AddressHelper ipv4;

    if(protocolName=="AODV"){
        list.Add (aodv, 100);//aodvルーティングヘルパーとその優先度(100)を格納する
        internet.SetRoutingHelper (list);//インストール時に使用するルーティングヘルパーを設定する
        internet.Install(adhocNodes);//各ノードに(Ipv4,Ipv6,Udp,Tcp)クラスの実装を集約する
    }
    else if(protocolName=="OLSR"){
        list.Add (olsr, 100);//olsrルーティングヘルパーとその優先度(100)を格納する
        internet.SetRoutingHelper (list);//インストール時に使用するルーティングヘルパーを設定する
        internet.Install(adhocNodes);//各ノードに(Ipv4,Ipv6,Udp,Tcp)クラスの実装を集約する
    }else if(protocolName=="GPSR"){
        list.Add (gpsr, 100);//dsdvルーティングヘルパーとその優先度(100)を格納する
        internet.SetRoutingHelper (list);//インストール時に使用するルーティングヘルパーを設定する
        internet.Install(adhocNodes);//各ノードに(Ipv4,Ipv6,Udp,Tcp)クラスの実装を集約する
    }

	ipv4.SetBase ("192.168.1.0", "255.255.255.0");
	adhocInterfaces = ipv4.Assign (adhocDevice);//IPアドレス割当

}

void 
VanetRoutingExperiment::ConfigureApplications ()
{
    uint32_t src,dst;
    uint32_t totalsrcdst = 1;//送受信ノードそれぞれの数

    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    for (uint32_t i=0; i<totalsrcdst; i++){
        src = sourceNode[i];
        dst = sinkNode[i];

        Ptr<Socket> destination = Socket::CreateSocket (adhocNodes.Get(dst), tid);
	      InetSocketAddress local = InetSocketAddress (adhocInterfaces.GetAddress(dst), Port);
  	    destination->Bind (local);
	      destination->SetRecvCallback (MakeCallback (&VanetRoutingExperiment::ReceivePacket,this));

        Ptr<Socket> source = Socket::CreateSocket (adhocNodes.Get(src), tid);
        InetSocketAddress srcSocketAddr = InetSocketAddress (adhocInterfaces.GetAddress(src), Port);
        if (InetSocketAddress::IsMatchingType (adhocInterfaces.GetAddress(dst)) || PacketSocketAddress::IsMatchingType (adhocInterfaces.GetAddress(dst))){
          source->Bind (srcSocketAddr); 
        }
        source->SetAllowBroadcast (true);
        source->Connect (local);

        Time interPacketInterval = Seconds (1);
        
        //Simulator::Schedule (Seconds (20.0), &VanetRoutingExperiment::SendPacket, this, source, interPacketInterval);
        Simulator::ScheduleWithContext (source->GetNode()->GetId(), Seconds (5.0), &VanetRoutingExperiment::SendPacket, this, source, interPacketInterval);
    }
}

void
VanetRoutingExperiment::SendPacket (Ptr<Socket> socket, Time pktInterval)
{
    NS_LOG_FUNCTION(this);

    if (numPacket > 0) {

        std::ostringstream msg;
        msg << "packet recive!";
        uint32_t size = msg.str().length();
        Ptr<Packet> packet = Create<Packet>((uint8_t *)msg.str().c_str(), size);
        socket->Send(packet);

		    //socket->Send (Create<Packet> (packetSize));
		    Simulator::Schedule (pktInterval, &VanetRoutingExperiment::SendPacket, this, socket, pktInterval);
		    sendPacket ++;
		    numPacket --;
    } 
    else {
		    socket->Close ();  
	  }
}

void 
VanetRoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    while ((packet = socket->Recv ()))
    {

      uint8_t *buffer = new uint8_t[packet->GetSize()];
      packet->CopyData(buffer, packet->GetSize());
      std::string s = std::string(buffer, buffer + packet->GetSize());
      std::cout << s << std::endl;
      delete buffer;

      receivePacket ++;
    }
}

void 
VanetRoutingExperiment::RunSimulation ()
{
    NS_LOG_INFO ("Run Simulation.");//メッセージ"Run Simulation"をログに記録する

    flowMonitorHelper = new FlowMonitorHelper;
    flowMonitor = flowMonitorHelper->InstallAll();//全てのノードにフローモニター装着

    Simulator::Stop (Seconds (totalSimTime));//シミュレーションが停止するまでの時間をスケジュールする

    Simulator::Run ();//シミュレーションを実行する
    RunFlowMonitor();

    Simulator::Destroy ();//シミュレーションの最後に呼び出す
}

void
VanetRoutingExperiment::RunFlowMonitor()
{
    int countFlow=0;
    double sumThroughput=0;
    uint64_t sumTxBytes=0;
    uint64_t sumRxBytes=0;
    uint32_t sumTimesForwarded=0;
    uint32_t sumLostPackets=0;
    uint32_t sumTxPackets=0;
    uint32_t sumRxPackets=0;
    Time sumDelay;

    uint64_t sumOverHead=0;
    
      Ptr<Ipv4FlowClassifier> flowClassifier = DynamicCast<Ipv4FlowClassifier> (flowMonitorHelper->GetClassifier ());//GetClassifierメソッドでFlowClassifierオブジェクトを取得
      //IPv4FlowClassifierにキャストする
      std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();//収集されたすべてのフロー統計を取得する

      for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i=stats.begin (); i != stats.end (); i++) //stats(フロー統計)の要素分ループする
      {

        Ipv4FlowClassifier::FiveTuple t = flowClassifier->FindFlow (i->first);//各stats(フロー統計)のフローIdに対応するFiveTupleを検索する
        //FiveTuple(宛先アドレス,送信元アドレス,宛先ポート番号,送信元ポート番号)

        if(t.destinationPort==Port)
        {//セッションの送信元IPアドレス,宛先アドレスが検索したものと一致した時

          countFlow ++;

          double throughput = double(i->second.rxBytes *8.0)/(i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ())/1024;
          //rxBytesはこのフローの受信バイトの合計数,timeLastRxPacketはフローで最後のパケットが受信されたときの時間
          //timeFirstTxPacketはフローで最初のパケットが送信された時の時間
          //throughput=受信パケットのバイト数 *8.0/通信時間/1024
          sumThroughput+= throughput;//スループットの合計

          sumTxBytes += i->second.txBytes;//txBytesはこのフローの送信バイトの合計数,

          sumRxBytes += i->second.rxBytes;//rxBytesはこのフローの受信バイトの合計数

          sumTimesForwarded += i->second.timesForwarded;//転送回数の合計(ホップ数の合計)

          sumLostPackets +=  i->second.lostPackets;//失われたとパケットの数

          sumTxPackets += i->second.txPackets;//txPacketsは送信パケットの合計

          sumRxPackets += i->second.rxPackets;//rxPacketsは受信パケットの合計

          sumDelay += i->second.delaySum;//delaySumはすべてのエンドツーエンド通信の遅延の合計

        }else{
          sumOverHead+=i->second.txBytes;
        }

      }

    m_throughput = sumThroughput;

    m_pdr = (double(sumRxBytes)/sumTxBytes)*100.0;

  m_overHead = ((double(m_wifiPhyStats->GetPhyTxBytes()-sumTxBytes))/m_wifiPhyStats->GetPhyTxBytes())*100;
    if(m_overHead<0.0)
      m_overHead=0.0;

    m_delay = ((sumDelay.GetSeconds()/sumRxPackets))*1000;
    if(std::isnan(m_delay))
      m_delay=0.0;

    m_packetLoss = (double(sumLostPackets)/sumTxPackets)*100;

    m_numHops = 1.0+double(sumTimesForwarded)/sumRxPackets;
    if(std::isnan(m_numHops))
      m_numHops=0.0;

    std::cout<<"スループット(kbps)"<<m_throughput<<std::endl;
    std::cout<<"配送率"<<m_pdr<<std::endl;
    std::cout<<"オーバーヘッド割合"<<m_overHead<<std::endl;
    std::cout<<"平均遅延(ms)"<<m_delay<<std::endl;
    std::cout<<"パケットロス率"<<m_packetLoss<<std::endl;
    std::cout<<"平均ホップ数"<<m_numHops<<std::endl;
    std::cout<<"フロー数"<<countFlow<<std::endl;
    std::cout<<"オーバーヘッドも含めた送信バイト合計"<<m_wifiPhyStats->GetPhyTxBytes()<<std::endl;
    std::cout<<std::endl;
    std::cout<<"データパケット----------------------"<<std::endl;
    std::cout<<"送信パケット合計"<<sendPacket<<std::endl;
    std::cout<<"受信パケット合計"<<receivePacket<<std::endl;
    std::cout<<"送信バイト合計:"<<sumTxBytes<<std::endl;
    std::cout<<"受信バイト合計:"<<sumRxBytes<<std::endl;
    std::cout<<"ホップ数合計:"<<sumTimesForwarded<<std::endl;
    std::cout<<"パケットロス合計"<<sumLostPackets<<std::endl;
    std::cout<<"遅延合計"<<sumDelay.GetSeconds()*1000<<"ms"<<std::endl;
    std::cout<<"送信パケット数合計"<<sumTxPackets<<std::endl;
    std::cout<<"受信パケット数合計"<<sumRxPackets<<std::endl;
    std::cout<<"送信オーバーヘッド合計"<<sumOverHead<<std::endl;
    
}

int main (int argc, char *argv[])
{
  VanetRoutingExperiment experiment;//パラメータの初期化
  experiment.Simulate (argc, argv);//シミュレーションを実行する
  return 0;
}

  