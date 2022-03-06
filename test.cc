#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-helper.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ssid.h"
#include "ns3/node-container.h"
#include "ns3/boolean.h"
#include "ns3/string.h"

using namespace ns3;

Ptr<PacketSink> sink;	/* pointer to sink app*/
double lastTotalRx = 0;	/* record the last total received bytes */


void CalculateThroughput()
{
	Time now = Simulator::Now();
	double totalRx = (sink->GetTotalRx() - lastTotalRx) * (double) 8 / 1e5; /* calculate Rx packets in Mbit/s */
	std::cout << now.GetSeconds() << "s: " << totalRx << "Mbit/s" << std::endl ;
	lastTotalRx = sink->GetTotalRx();
	Simulator::Schedule(MilliSeconds(100), &CalculateThroughput);
}

/** 
 * @brief use specific WiFi standard and PHY rate to run simulation, and calculating the throughput/delay 
 * 
 * param standard which WiFi standard the simulation will use
 * param phyRate which MCS the WiFi PHY will use (1 spatial stream)
 *
 */
void RunSimulation(std::string standard, std::string phyRate)
{
	/* log or not */
	bool verbose = true;				/* log or not */
	bool calculateThroughputPerSecond = false;	/* run CalculateThroughput or not*/
	
	/* topology parameters */
	uint32_t nSta = 1;
	uint32_t nAp = 1;
	Ssid ssid = Ssid("Lab410");
	
	/* Application Layer setting */
	uint32_t port = 77;
	uint32_t packetSize = 1024;	/* size of application layer packets (min = 12 bytes) */
	double simulationTime = 10.0;		/* simulation duration */
	
	/* use which application */
	bool udpClientServer = false;	/* use udp-client-server-helper => can trace delay */
	bool onOffApplication = true;	/* use on-off application => can use packet-sink */
	
	/* udp-client-server parameters */
	uint32_t maxPackets = 10;	/* maximum number of packets the application will send */
	double packetInterval = 0.5;	/* the time to wait between packets */
	
	/* on-off-helper parameters */
	uint32_t maxBytes = 0; 		/* total number of bytes to send, 0 means no limit */
	std::string appDataRate = "500Mbps";
	std::string onTime = "ns3::ConstantRandomVariable[Constant=1.0]";	/* duration of on state */
	std::string offTime = "ns3::ConstantRandomVariable[Constant=0.0]";	/* duration of off state */
		

	if(verbose)
	{
		//LogComponaentEnable("UdpClient", LOG_LEVEL_INFO);
		LogComponentEnable("UdpServer", LOG_LEVEL_INFO);	/* use UDP timestamp trace delay */
		//LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);	/* use UDP timestamp trace delay */
	}

	/* create nodes */
	NodeContainer staNodes;
	NodeContainer apNodes;
	staNodes.Create(nSta);
	apNodes.Create(nAp);
	
	/* create wifi channel */
	YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
	
	/* create wifi phy */
	YansWifiPhyHelper phy;
	phy.SetChannel(channel.Create());

	/* create wifi mac */
	WifiMacHelper staMac;
	WifiMacHelper apMac;
	staMac.SetType("ns3::StaWifiMac",
		       "ActiveProbing", BooleanValue(false),
		       "Ssid", SsidValue(ssid));
	apMac.SetType("ns3::ApWifiMac",
		       "Ssid", SsidValue(ssid));


	/* create wifi helper */
	WifiHelper wifi;
	std::string controlPhyRate;	/* dataRate of control frame */
	if(standard == "11n_2_4"){
		wifi.SetStandard(WIFI_STANDARD_80211n_2_4GHZ);
		controlPhyRate = "HtMcs0";
	}
	else if(standard == "11n_5"){
		wifi.SetStandard(WIFI_STANDARD_80211n_5GHZ);
		controlPhyRate = "HtMcs0";
	}
	else if(standard == "11ac"){
		wifi.SetStandard(WIFI_STANDARD_80211ac);
		controlPhyRate = "VhtMcs0";
	}
	else if(standard == "11ax_2_4"){
		wifi.SetStandard(WIFI_STANDARD_80211ax_2_4GHZ);
		controlPhyRate = "HeMcs0";
	}
	else if(standard == "11ax_5"){
		wifi.SetStandard(WIFI_STANDARD_80211ax_5GHZ);
		controlPhyRate = "HeMcs0";
	}
	else if(standard == "11ax_6"){
		wifi.SetStandard(WIFI_STANDARD_80211ax_6GHZ);
		controlPhyRate = "HeMcs0";
	}

	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
				     "DataMode", StringValue(phyRate),
				     "ControlMode", StringValue(controlPhyRate));
	
	/* create devices */
	NetDeviceContainer staDevices;
	NetDeviceContainer apDevices;
	staDevices = wifi.Install(phy, staMac, staNodes);
	apDevices = wifi.Install(phy, apMac, apNodes);

	/* create mobility model */
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(staNodes);
	mobility.Install(apNodes);

	/* create Internet stack */
	InternetStackHelper stack;
	stack.Install(staNodes);
	stack.Install(apNodes);

	/* Ipv4 address setting */
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.0.1.0", "255.255.255.0");
	
	Ipv4InterfaceContainer staAddr;
	Ipv4InterfaceContainer apAddr;
	staAddr = ipv4.Assign(staDevices);
	apAddr = ipv4.Assign(apDevices);

	/* use UDP client/server => can use packet timestamp to trace delay */
	if(udpClientServer == true)
	{
		/* create UDP server on AP to receive packets */
		UdpServerHelper udpServer;
		udpServer.SetAttribute("Port", UintegerValue(port));

		ApplicationContainer serverApp;			/* server app receive packets from client */
		serverApp = udpServer.Install(apNodes);
		serverApp.Start(Seconds(0.1));			/* AP start to receive packets */
		serverApp.Stop(Seconds(simulationTime));	/* AP stop receiving packets */
		
		/* create UDP client on STAs to send packets to AP */
		UdpClientHelper udpClient;
		udpClient.SetAttribute("RemoteAddress", AddressValue(apAddr.GetAddress(0)));
		udpClient.SetAttribute("RemotePort", UintegerValue(port));
		udpClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
		udpClient.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
		udpClient.SetAttribute("PacketSize", UintegerValue(packetSize));

		ApplicationContainer clientApp;			/* client app send packets to server*/
		clientApp = udpClient.Install(staNodes);
		clientApp.Start(Seconds(0.3));			/* STAs start to send packets */
		clientApp.Stop(Seconds(simulationTime));	/* STAs stop sending packets */
	}
	if(onOffApplication == true)
	{
		/* create sink-application on AP to receive packets from STAs */
		PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(apAddr.GetAddress(0), port));

		ApplicationContainer serverApp;
		serverApp = sinkHelper.Install(apNodes);
		serverApp.Start(Seconds(0.0));				/* AP start to receive packets */
		//serverApp.Stop(Seconds(simulationTime));		/* AP stop receiving packets */
		
		sink = StaticCast<PacketSink>(serverApp.Get(0));	/* pointer of sinkHelper*/	
		
		/* create on-off-server on STAS to send packets to AP */
		OnOffHelper onOffServer("ns3::UdpSocketFactory",InetSocketAddress(apAddr.GetAddress(0), port));
		onOffServer.SetAttribute("DataRate", DataRateValue(DataRate(appDataRate)));
		onOffServer.SetAttribute("PacketSize", UintegerValue(packetSize));
		onOffServer.SetAttribute("OnTime", StringValue(onTime));
		onOffServer.SetAttribute("OffTime", StringValue(offTime));
		onOffServer.SetAttribute("MaxBytes", UintegerValue(maxBytes));
		
		ApplicationContainer clientApp;			/* client app send packets to server*/
		clientApp = onOffServer.Install(staNodes);
		clientApp.Start(Seconds(1.0));			/* STAs start to send packets */
		//clientApp.Stop(Seconds(simulationTime));	/* STAs stop sending packets */
	}
	
	
	/* Ipv4 routing table */
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	
	/* start simulation */
	if(calculateThroughputPerSecond == true)
	{
		Simulator::Schedule(Seconds(1.1), &CalculateThroughput);
	}
	Simulator::Stop(Seconds(simulationTime + 1));
	Simulator::Run();
	

	/* calculate throughput */
	if(onOffApplication == true)
	{
		double averageThroughput = ((sink->GetTotalRx() * 8) / (1e6 * simulationTime));
		std::cout  << standard << "\t\t" << phyRate << "\t\t\t";
		std::cout  << averageThroughput << std::endl;
	}

	Simulator::Destroy();
}

int main()
{
	/* WiFi standard list:
	 * 	WIFI_STANDARD_80211n_2_4GHZ	11n_2_4
	 * 	WIFI_STANDARD_80211n_5GHZ	11n_5
	 * 	WIFI_STANDARD_80211ac		11ac
	 * 	WIFI_STANDARD_80211ax_2_4GHZ	11ax_2_4
	 * 	WIFI_STANDARD_80211ax_5GHZ	11ax_5
	 * 	WIFI_STANDARD_80211ax_6GHZ	11ax_6
	 * */
	std::string standardList[6] = { "11n_2_4", 
				        "11n_5",
				        "11ac",
				        "11ax_2_4",
					"11ax_5",
					"11ax_6"};

	std::string HtMcs[8] =
        {
                "HtMcs0", "HtMcs1", "HtMcs2", "HtMcs3", "HtMcs4",
                "HtMcs5", "HtMcs6", "HtMcs7"
        };	
	std::string VhtMcs[10] = 
	{
		"VhtMcs0", "VhtMcs1", "VhtMcs2", "VhtMcs3", "VhtMcs4",
		"VhtMcs5", "VhtMcs6", "VhtMcs7", "VhtMcs8", "VhtMcs9"
	};
	std::string HeMcs[12] = 
	{
		"HeMcs0", "HeMcs1", "HeMcs2", "HeMcs3", "HeMcs4", "HeMcs5",
		"HeMcs6", "HeMcs7", "HeMcs8", "HeMcs9", "HeMcs10", "HeMcs11"
	};
	
	std::cout << "WiFi Standard\t" << "MCS\t";
	std::cout << "Average throughput (Mbits/s)" << std::endl;
	
	uint8_t j;
	for(j = 0; j < 8; j++)
	{
		RunSimulation(standardList[0], HtMcs[j]);
	}
	for(j = 0; j < 8; j++)
	{
		RunSimulation(standardList[1], HtMcs[j]);
	}
	for(uint8_t j = 0; j <10; j++)
	{
		RunSimulation(standardList[2], VhtMcs[j]);
	}
	for(uint8_t j = 0; j < 12; j++)
	{
		RunSimulation(standardList[3], HeMcs[j]);
	}
	for(uint8_t j = 0; j < 12; j++)
	{
		RunSimulation(standardList[4], HeMcs[j]);
	}
	for(uint8_t j = 0; j < 12; j++)
	{
		RunSimulation(standardList[5], HeMcs[j]);
	}
	
	
	
	return 0;	
}
