#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/spectrum-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/spectrum-analyzer-helper.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/boolean.h"

using namespace ns3;

Ptr<PacketSink> sink; /* pointer to sink app */
double lastTotalRx = 0; /* record the last total received bytes */


void CalculateThroughput()
{
	Time now = Simulator::Now();
	double totalRx = ((sink->GetTotalRx() - lastTotalRx) * (double)8 / 1e5);	
	/* calculate throughput 10times per second */ 
	std::cout << "Time: "<< now.GetSeconds() << "s Throughput: " << totalRx << " Mbps" << std::endl;
	lastTotalRx = sink->GetTotalRx();
	Simulator::Schedule(MilliSeconds(100), &CalculateThroughput);

}

int main()
{
	uint8_t nSta = 1;
	uint8_t nAp = 1;
	uint32_t simulationDurationTime = 500 * 1e3;	/* micro seconds */
	uint32_t simulationStartTime = 100000;		/* micro seconds */
	bool calculateThroughputOrNot = false;
	bool verbose = false;
	
	if(verbose)
	{
		//LogComponentEnable("SpectrumWifiPhy", LOG_LEVEL_INFO);
		//LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
		LogComponentEnable("SpectrumWifiPhy", LOG_LEVEL_INFO);
		//LogComponentEnable("WifiSpectrumValueHelper", LOG_LEVEL_ALL);
	}

	NodeContainer staNodes;
	NodeContainer apNodes;
	NodeContainer spectrumAnalyzerNodes;
	staNodes.Create(nSta);
	apNodes.Create(nAp);
	spectrumAnalyzerNodes.Create(1);

	/* create spectrum channel */
	SpectrumChannelHelper channelHelper = SpectrumChannelHelper::Default();
	channelHelper.SetChannel("ns3::MultiModelSpectrumChannel");
	channelHelper.AddSpectrumPropagationLoss("ns3::ConstantSpectrumPropagationLossModel");
	Ptr<SpectrumChannel> channel = channelHelper.Create(); 

	/* create spectrum PHY */
	uint32_t frequency = 5180;
	uint32_t bandwidth = 20;
	double power = 23.0;

	SpectrumWifiPhyHelper spectrumPhy;
	spectrumPhy.SetChannel(channel);
	spectrumPhy.SetErrorRateModel("ns3::NistErrorRateModel");
	spectrumPhy.Set("Frequency", UintegerValue(frequency));
	spectrumPhy.Set("ChannelWidth", UintegerValue(bandwidth));
	spectrumPhy.Set("TxPowerStart", DoubleValue(power));
	spectrumPhy.Set("TxPowerEnd", DoubleValue(power));

	/* create wifi helper */
	WifiHelper wifi;
	wifi.SetStandard(WIFI_STANDARD_80211ax_5GHZ);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
				     "DataMode", StringValue("HeMcs0"),
				     "ControlMode", StringValue("HeMcs0"));

	WifiMacHelper mac;
	NetDeviceContainer staDevice;
	NetDeviceContainer apDevice;
	NetDeviceContainer spectrumAnalyzerDevice;

	/* create STA/AP devices */
	Ssid ssid = Ssid("Lab410");
	mac.SetType(	"ns3::StaWifiMac",
			"Ssid", SsidValue(ssid),
			"ActiveProbing", BooleanValue(false));
	staDevice = wifi.Install(spectrumPhy, mac, staNodes);

	mac.SetType(	"ns3::ApWifiMac",
			"Ssid", SsidValue(ssid),
			"EnableBeaconJitter", BooleanValue(false));
	apDevice = wifi.Install(spectrumPhy, mac, apNodes);

	/* mobility setting */
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(staNodes);
	mobility.Install(apNodes);
	mobility.Install(spectrumAnalyzerNodes);

	/* create spectrum model */
	/* 
	 * channel will span (channel width + 2 * guardBandwidth) MHz
	 *
	 *
	 * */
	uint32_t centerFrequency = 5180;
	uint16_t channelWidth = 100;
	uint16_t guardBandwidth = 2;
	uint32_t bandBandwidth = 2000000;	/* width of 5GHz ISM band */

	WifiSpectrumValueHelper spectrumValueHelper;
	Ptr<SpectrumModel> spectrumAnalyzerFreqModel = spectrumValueHelper.GetSpectrumModel(	centerFrequency,
												channelWidth,
												bandBandwidth,
												guardBandwidth);
	
	/* spectrum analyzer setup  */
	SpectrumAnalyzerHelper spectrumAnalyzerHelper;
	spectrumAnalyzerHelper.SetChannel(channel);
	spectrumAnalyzerHelper.SetRxSpectrumModel(spectrumAnalyzerFreqModel);
	spectrumAnalyzerHelper.SetPhyAttribute("Resolution", TimeValue(MicroSeconds(4)));
	spectrumAnalyzerHelper.EnableAsciiAll("test.cc");
	spectrumAnalyzerDevice = spectrumAnalyzerHelper.Install(spectrumAnalyzerNodes);
	


	/* Internet stack */
	InternetStackHelper stack;
	stack.Install(staNodes);
	stack.Install(apNodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.0.0", "255.255.255.0");

	Ipv4InterfaceContainer staAddr;
	Ipv4InterfaceContainer apAddr;
	staAddr = ipv4.Assign(staDevice);
	apAddr = ipv4.Assign(apDevice);


	/* create sink server to receive packets from STAs */
	uint8_t port = 77;
	PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(apAddr.GetAddress(0), port));
		
	ApplicationContainer serverApp;
	serverApp = sinkHelper.Install(apNodes);
	serverApp.Start(MicroSeconds(0));
	sink = StaticCast<PacketSink>(serverApp.Get(0));

	/* create UDP clients to send packets to AP */
	uint32_t packetSize = 1024;
	uint32_t maxBytes = 0;
	std::string appDataRate = "500Mbps";
	std::string onTime = "ns3::ConstantRandomVariable[Constant=1.0]";
	std::string offTime = "ns3::ConstantRandomVariable[Constant=0.0]";

	OnOffHelper onOffServer("ns3::UdpSocketFactory", InetSocketAddress(apAddr.GetAddress(0), port));
	onOffServer.SetAttribute("DataRate", DataRateValue(appDataRate));
	onOffServer.SetAttribute("PacketSize", UintegerValue(packetSize));
	onOffServer.SetAttribute("OnTime", StringValue(onTime));
	onOffServer.SetAttribute("OffTime", StringValue(offTime));
	onOffServer.SetAttribute("MaxBytes", UintegerValue(maxBytes));

	ApplicationContainer clientApp;
	clientApp = onOffServer.Install(staNodes);
	clientApp.Start(MicroSeconds(simulationStartTime));

	/* start simulation */
	if(calculateThroughputOrNot){
		Simulator::Schedule(MicroSeconds(simulationStartTime), &CalculateThroughput);
	}
	Simulator::Stop(MicroSeconds((simulationStartTime + simulationDurationTime)));
	Simulator::Run();
	
	/* print throughput analyze */
	double averageThroughput = ((sink->GetTotalRx() * 8) / (double)(simulationDurationTime));
	std::cout << "Average Throughput: " << averageThroughput << " Mbps" << std::endl;

	Simulator::Destroy();

	return 0;
}
