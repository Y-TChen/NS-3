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

Ptr<PacketSink> sink1; /* pointer to sink app 1 */
Ptr<PacketSink> sink2; /* pointer to sink app 2 */
double lastTotalRx1 = 0; /* record the last total received bytes */
double lastTotalRx2 = 0; /* record the last total received bytes */


void CalculateThroughput()
{
	Time now = Simulator::Now();
	double totalRx1 = ((sink1->GetTotalRx() - lastTotalRx1) * (double)8 / 1e4);	
	double totalRx2 = ((sink2->GetTotalRx() - lastTotalRx2) * (double)8 / 1e4);	
	/* calculate throughput every 10 MilliSeconds */ 
	std::cout << now.GetSeconds() << "\t" << totalRx1 << "\t" << totalRx2 << std::endl;
	lastTotalRx1 = sink1->GetTotalRx();
	lastTotalRx2 = sink2->GetTotalRx();
	Simulator::Schedule(MicroSeconds(10 * 1000), &CalculateThroughput);
}

int main()
{
	uint8_t nSta = 2;
	uint8_t nAp = 2;
	uint32_t simulationDurationTime = 100 * 1000;	/* micro seconds */
	uint32_t simulationStartTime = 150 * 1000;	/* micro seconds */
	uint32_t simulationOffset = 50 * 1000;		/* micro seconds */
	bool calculateThroughputOrNot = true;
	uint32_t analyzerResolution = 80;		/* every 40 micro seconds analyze spectrum one time */
	bool verbose = false;
	
	if(verbose)
	{
		//LogComponentEnable("SpectrumWifiPhy", LOG_LEVEL_INFO);
		//LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
		LogComponentEnable("SpectrumWifiPhy", LOG_LEVEL_INFO);
		//LogComponentEnable("WifiSpectrumValueHelper", LOG_LEVEL_ALL);
	}

	/* create nodes */
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

	/* create wifi helper */
	WifiHelper wifi;
	wifi.SetStandard(WIFI_STANDARD_80211ax_5GHZ);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
				     "DataMode", StringValue("HeMcs0"),
				     "ControlMode", StringValue("HeMcs0"));

	/* create devices */
	NetDeviceContainer staDevice1;
	NetDeviceContainer apDevice1;
	NetDeviceContainer staDevice2;
	NetDeviceContainer apDevice2;
	NetDeviceContainer spectrumAnalyzerDevice;
	
	/*************************** create BSS 1 *****************************/
	/* create spectrum PHY 1 */
	uint32_t frequency1 = 5180 + 3 * 20;
	uint32_t bandwidth1 = 20;
	double power1 = 23.0;

	SpectrumWifiPhyHelper spectrumPhy1;
	spectrumPhy1.SetChannel(channel);
	spectrumPhy1.SetErrorRateModel("ns3::NistErrorRateModel");
	spectrumPhy1.Set("Frequency", UintegerValue(frequency1));
	spectrumPhy1.Set("ChannelWidth", UintegerValue(bandwidth1));
	spectrumPhy1.Set("TxPowerStart", DoubleValue(power1));
	spectrumPhy1.Set("TxPowerEnd", DoubleValue(power1));

	/* create STA/AP devices 1 */
	WifiMacHelper mac1;
	Ssid ssid1 = Ssid("Lab410_1");
	mac1.SetType(	"ns3::StaWifiMac",
			"Ssid", SsidValue(ssid1),
			"ActiveProbing", BooleanValue(false));
	staDevice1 = wifi.Install(spectrumPhy1, mac1, staNodes.Get(0));

	mac1.SetType(	"ns3::ApWifiMac",
			"Ssid", SsidValue(ssid1),
			"EnableBeaconJitter", BooleanValue(false));
	apDevice1 = wifi.Install(spectrumPhy1, mac1, apNodes.Get(0));

	/*************************** create BSS 2 *****************************/
	/* create spectrum PHY 2 */
	uint32_t frequency2 = 5180;
	uint32_t bandwidth2 = 20;
	double power2 = 23.0;

	SpectrumWifiPhyHelper spectrumPhy2;
	spectrumPhy2.SetChannel(channel);
	spectrumPhy2.SetErrorRateModel("ns3::NistErrorRateModel");
	spectrumPhy2.Set("Frequency", UintegerValue(frequency2));
	spectrumPhy2.Set("ChannelWidth", UintegerValue(bandwidth2));
	spectrumPhy2.Set("TxPowerStart", DoubleValue(power2));
	spectrumPhy2.Set("TxPowerEnd", DoubleValue(power2));

	/* create STA/AP devices 2 */
	WifiMacHelper mac2;
	Ssid ssid2 = Ssid("Lab410_2");
	mac2.SetType(	"ns3::StaWifiMac",
			"Ssid", SsidValue(ssid2),
			"ActiveProbing", BooleanValue(false));
	staDevice2 = wifi.Install(spectrumPhy2, mac2, staNodes.Get(1));

	mac2.SetType(	"ns3::ApWifiMac",
			"Ssid", SsidValue(ssid2),
			"EnableBeaconJitter", BooleanValue(false));
	apDevice2 = wifi.Install(spectrumPhy2, mac2, apNodes.Get(1));


	/* mobility setting */
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(staNodes);
	mobility.Install(apNodes);
	mobility.Install(spectrumAnalyzerNodes);

	/* Internet stack */
	InternetStackHelper stack;
	stack.Install(staNodes);
	stack.Install(apNodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.0.0", "255.255.255.0");

	Ipv4InterfaceContainer staAddr1, staAddr2;
	Ipv4InterfaceContainer apAddr1, apAddr2;
	staAddr1 = ipv4.Assign(staDevice1);
	apAddr1 = ipv4.Assign(apDevice1);
	staAddr2 = ipv4.Assign(staDevice2);
	apAddr2 = ipv4.Assign(apDevice2);

	/************** create sink server 1 to receive packets from STAs 1 ****************************/
	uint8_t port1 = 77;
	PacketSinkHelper sinkHelper1("ns3::UdpSocketFactory", InetSocketAddress(apAddr1.GetAddress(0), port1));
		
	ApplicationContainer serverApp1;
	serverApp1 = sinkHelper1.Install(apNodes.Get(0));
	serverApp1.Start(MicroSeconds(0));
	sink1 = StaticCast<PacketSink>(serverApp1.Get(0));

	/*************** create sink server 2 to receive packets from STAs 2 *******************/
	uint8_t port2 = 78;
	PacketSinkHelper sinkHelper2("ns3::UdpSocketFactory", InetSocketAddress(apAddr2.GetAddress(0), port2));
		
	ApplicationContainer serverApp2;
	serverApp2 = sinkHelper2.Install(apNodes.Get(1));
	serverApp2.Start(MicroSeconds(0));
	sink2 = StaticCast<PacketSink>(serverApp2.Get(0));
	
	/* create UDP clients to send packets to AP */
	uint32_t packetSize = 1024;
	uint32_t maxBytes = 0;
	std::string appDataRate = "500Mbps";
	std::string onTime = "ns3::ConstantRandomVariable[Constant=1.0]";
	std::string offTime = "ns3::ConstantRandomVariable[Constant=0.0]";

	OnOffHelper onOffServer1("ns3::UdpSocketFactory", InetSocketAddress(apAddr1.GetAddress(0), port1));
	onOffServer1.SetAttribute("DataRate", DataRateValue(appDataRate));
	onOffServer1.SetAttribute("PacketSize", UintegerValue(packetSize));
	onOffServer1.SetAttribute("OnTime", StringValue(onTime));
	onOffServer1.SetAttribute("OffTime", StringValue(offTime));
	onOffServer1.SetAttribute("MaxBytes", UintegerValue(maxBytes));
	
	OnOffHelper onOffServer2("ns3::UdpSocketFactory", InetSocketAddress(apAddr2.GetAddress(0), port2));
	onOffServer2.SetAttribute("DataRate", DataRateValue(appDataRate));
	onOffServer2.SetAttribute("PacketSize", UintegerValue(packetSize));
	onOffServer2.SetAttribute("OnTime", StringValue(onTime));
	onOffServer2.SetAttribute("OffTime", StringValue(offTime));
	onOffServer2.SetAttribute("MaxBytes", UintegerValue(maxBytes));

	/**************************** create APP1 *********************************/
	ApplicationContainer clientApp1;
	clientApp1 = onOffServer1.Install(staNodes.Get(0));
	clientApp1.Start(MicroSeconds(simulationStartTime));	/* 100 * 1000 MicroSeconds */
	
	/**************************** create APP2 *********************************/
	ApplicationContainer clientApp2;
	clientApp2 = onOffServer2.Install(staNodes.Get(1));
	clientApp2.Start(MicroSeconds(simulationStartTime + simulationOffset));

	/* create spectrum model */
	uint32_t centerFrequency = (frequency1 + frequency2) / 2;
	uint16_t channelWidth = (frequency1 - frequency2) + 4 * 20;
	uint16_t guardBandwidth = 2;
	uint32_t bandBandwidth = 2000000;	/* in Hz */

	/* Spectrum Analyzer setup */
	WifiSpectrumValueHelper spectrumValueHelper;
	Ptr<SpectrumModel> spectrumAnalyzerFreqModel = spectrumValueHelper.GetSpectrumModel(	centerFrequency,
												channelWidth,
												bandBandwidth,
												guardBandwidth);
	SpectrumAnalyzerHelper spectrumAnalyzerHelper;
	spectrumAnalyzerHelper.SetChannel(channel);
	spectrumAnalyzerHelper.SetRxSpectrumModel(spectrumAnalyzerFreqModel);
	spectrumAnalyzerHelper.SetPhyAttribute("Resolution", TimeValue(MicroSeconds(analyzerResolution)));
	spectrumAnalyzerHelper.EnableAsciiAll("TEST000");
	spectrumAnalyzerDevice = spectrumAnalyzerHelper.Install(spectrumAnalyzerNodes);
	

	/* start simulation */
	if(calculateThroughputOrNot){
		std::cout << "Time(s)" << " Throughput_1" << "Throughput_2" << std::endl;
		Simulator::Schedule(MicroSeconds(0), &CalculateThroughput);
	}
	Simulator::Stop(MicroSeconds((simulationStartTime + simulationOffset + simulationDurationTime)));
	Simulator::Run();
	
	/* print throughput analyze */
	double averageThroughput1 = ((sink1->GetTotalRx() * 8) / (double)(simulationDurationTime + simulationOffset));
	double averageThroughput2 = ((sink2->GetTotalRx() * 8) / (double)(simulationDurationTime));
	std::cout << "\nAverage Throughput\nAP 1\tAP 2\n" << averageThroughput1 << "\t" << averageThroughput2 << std::endl;

	Simulator::Destroy();

	return 0;
}
