#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-helper.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/udp-client-server-helper.h"

#include "ns3/ssid.h"
#include "ns3/node-container.h"
#include "ns3/boolean.h"
#include "ns3/string.h"

using namespace ns3;

int main()
{
	uint32_t nSta = 1;
	uint32_t nAp = 1;
	Ssid ssid = Ssid("Lab410");
	
	/* Application Layer setting */
	uint32_t maxPackets = 15;	/* maximum number of packets the application will send */
	uint32_t packetSize = 1024;	/* size of application layer packets (min = 12 bytes) */
	double packetInterval = 1;	/* the time to wait between packets */
	double stopTime = 10.0;		/* simulation stop time */
	
	bool udpClientServer = true;	/* use udp-client-server-helper => can trace delay */
	bool verbose = true;

	/* WiFi standard list:
	 * 	WIFI_STANDARD_80211a		11a
	 * 	WIFI_STANDARD_80211b		11b
	 * 	WIFI_STANDARD_80211g		11g
	 * 	WIFI_STANDARD_80211n_2_4GHZ	11n_2_4
	 * 	WIFI_STANDARD_80211n_5GHZ	11n_5
	 * 	WIFI_STANDARD_80211ac		11ac
	 * 	WIFI_STANDARD_80211ax_2_4GHZ	11ax_2_4
	 * 	WIFI_STANDARD_80211ax_5GHZ	11ax_5
	 * 	WIFI_STANDARD_80211ax_6GHZ	11ax_6
	 * */
	std::string standard = "11a";
	

	if(verbose)
	{
		//LogComponaentEnable("UdpClient", LOG_LEVEL_INFO);
		LogComponentEnable("UdpServer", LOG_LEVEL_INFO);	/* use UDP timestamp trace delay */
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
	std::string phyRate;
	if(standard == "11a"){
		wifi.SetStandard(WIFI_STANDARD_80211a);
		phyRate = "OfdmRate6Mbps";
	}
	else if(standard == "11n_2_4"){
		wifi.SetStandard(WIFI_STANDARD_80211n_2_4GHZ);
		phyRate = "HtMcs0";
	}
	else if(standard == "11n_5"){
		wifi.SetStandard(WIFI_STANDARD_80211n_5GHZ);
		phyRate = "HtMcs0";
	}
	else if(standard == "11ac"){
		wifi.SetStandard(WIFI_STANDARD_80211ac);
		phyRate = "VhtMcs0";
	}
	else if(standard == "11ax_2_4"){
		wifi.SetStandard(WIFI_STANDARD_80211ax_2_4GHZ);
		phyRate = "HeMcs0";
	}
	else if(standard == "11ax_5"){
		wifi.SetStandard(WIFI_STANDARD_80211ax_5GHZ);
		phyRate = "HeMcs0";
	}
	else if(standard == "11ax_6"){
		wifi.SetStandard(WIFI_STANDARD_80211ax_6GHZ);
		phyRate = "HeMcs0";
	}

	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
				     "DataMode", StringValue(phyRate),
				     "ControlMode", StringValue(phyRate));
	
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
		/* create UDP server and client */
		uint32_t port = 77;
		UdpServerHelper udpServer;
		udpServer.SetAttribute("Port", UintegerValue(port));

		UdpClientHelper udpClient;
		udpClient.SetAttribute("RemoteAddress", AddressValue(apAddr.GetAddress(0)));
		udpClient.SetAttribute("RemotePort", UintegerValue(port));
		udpClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
		udpClient.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
		udpClient.SetAttribute("PacketSize", UintegerValue(packetSize));

		/* create application of server and client */
		ApplicationContainer serverApp;		/* server app receive packets from client */
		serverApp = udpServer.Install(apNodes);
		serverApp.Start(Seconds(1.0));		/* AP start to receive packets */
		serverApp.Stop(Seconds(stopTime));	/* AP stop receiving packets */


		ApplicationContainer clientApp;		/* client app send packets to server*/
		clientApp = udpClient.Install(staNodes);
		clientApp.Start(Seconds(7.0));		/* STAs start to send packets */
		clientApp.Stop(Seconds(stopTime));	/* STAs stop sending packets */
	}
	/* Ipv4 routing table */
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	
	/* start simulation */
	Simulator::Stop(Seconds(stopTime));
	Simulator::Run();
	Simulator::Destroy();


	return 0;
}
