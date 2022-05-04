/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/multi-link-device.h"
#include "ns3/nstime.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MultiLinkDevice");

NS_OBJECT_ENSURE_REGISTERED (MultiLinkDevice);

TypeId
MultiLinkDevice::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::MultiLinkDevice")
        .SetParent<Object> ();

        return tid;
}


MultiLinkDevice::MultiLinkDevice() 
    : m_totalByte (0),
      m_totalReceive (0),
      m_linkNumber(0),
      m_transitFreq (MicroSeconds(100 * 1000)),
      m_transitDelay (MicroSeconds(128)),
      m_isTransit(false),
      m_SendError(0),
      m_cbrRate (DataRate("10Mb/s")),
      m_packetSize (1024),
      m_residualBits (0),
      m_lastStartTime (Seconds(0)),
      m_unsentPacket (0),
      m_isAP (true)
      
{

}

MultiLinkDevice::~MultiLinkDevice()
{}

void
MultiLinkDevice::SetSTA1(const NetDeviceContainer device)
{
    Ptr<WifiNetDevice> wd = DynamicCast<WifiNetDevice>(device.Get(0));
    m_sta1 = wd;
}

Ptr<WifiNetDevice>
MultiLinkDevice::GetSTA1()
{
    return m_sta1;
}

void
MultiLinkDevice::SetSTA2(const NetDeviceContainer device)
{
    Ptr<WifiNetDevice> wd = DynamicCast<WifiNetDevice>(device.Get(0));
    m_sta2 = wd;
}

Ptr<WifiNetDevice>
MultiLinkDevice::GetSTA2()
{
    return m_sta2;
}

Address 
MultiLinkDevice::GetAddress1()
{
    return m_sta1->GetAddress();
}

Address 
MultiLinkDevice::GetAddress2()
{
    return m_sta2->GetAddress();
}

uint32_t 
MultiLinkDevice::GetTotalByte()
{
    return m_totalByte;
}

void 
MultiLinkDevice::Clear()
{
    m_isTransit = false;
}

void 
MultiLinkDevice::SetTransitDelay(Time delay)
{
    m_transitDelay = delay;
}

Time
MultiLinkDevice::GetTransitDelay()
{
    return m_transitDelay;
}

Time
MultiLinkDevice::GetTransitFreq()
{
    return m_transitFreq;
}

void
MultiLinkDevice::SetTransitFreq(Time freq)
{
    m_transitFreq = freq;
}

uint32_t
MultiLinkDevice::GetSendError()
{
    return m_SendError;
}

void 
MultiLinkDevice::SocketSetting(Ptr<Socket> socket1, Ptr<Socket> socket2, Address addr1, Address addr2, DataRate cbrRate, bool isAP)
{
    m_socket1 = socket1;
    m_socket2 = socket2;
    m_socket1->Connect(addr1);
    m_socket2->Connect(addr2);
    m_cbrRate = cbrRate;
    m_isAP = isAP;


    /* if this device is STA => start to transmit packets*/
    if(m_isAP == false)
    {
        Simulator::Schedule(m_transitFreq, &MultiLinkDevice::SwitchLink, this);
        Simulator::ScheduleNow (&MultiLinkDevice::SchduleNextTx, this);
    }
}

void 
MultiLinkDevice::SchduleNextTx()
{
    NS_ABORT_MSG_IF (m_residualBits > m_packetSize * 8, "Calculation to compute next send time will overflow");
    /* calculate remain bits */
    uint32_t bits = m_packetSize * 8 - m_residualBits;  
    /* calculate next send time  */
    Time nextTime = (Seconds(bits/ static_cast<double>(m_cbrRate.GetBitRate())));
    Simulator::Schedule(nextTime, &MultiLinkDevice::SendPacket, this); 
}

void 
MultiLinkDevice::SendPacket ()
{

    /* check the link state, sending failed if under transiting state */
    if(m_isTransit == true)
    {
        NS_LOG_INFO("[Send] Sending error, device is under transiting state.");
        m_SendError++;
        Simulator::Schedule (MicroSeconds(10), &MultiLinkDevice::SendPacket, this);
        return;
    }

    Ptr<Socket> socket;
    Ptr<Packet> packet;
    /* see whether have unsent packet */
    if(m_unsentPacket)
    {
        packet = m_unsentPacket;
    }
    else
    {
        packet = Create<Packet> (m_packetSize);
    }

    /* decide use which STA(socket) to sned packet  */
    socket = (m_linkNumber == 0)? m_socket1 : m_socket2;
    int actual = socket->Send(packet);
    /* check how many data have been successfully transmitted */
    if((unsigned) actual == m_packetSize)
    {
        m_totalByte += m_packetSize;
        m_unsentPacket = 0;
    }
    else
    {
        /* cached remain data for letter send */
        m_unsentPacket = packet;
    }

    m_residualBits = 0;
    m_lastStartTime = Simulator::Now();
    SchduleNextTx();
}

void 
MultiLinkDevice::SwitchLink()
{
    NS_LOG_INFO("[ Transiting... ]");
    /* change the state of this device */
    m_isTransit = true;
    /* switch link */
    if(m_linkNumber == 0)
    {
        m_linkNumber = 1;
    }
    else
    {
        m_linkNumber = 0;
    }
    /* clear the transit flag after tansition delay */
    Simulator::Schedule(m_transitDelay, &MultiLinkDevice::Clear, this);
    /* switch link every transit frequency */
    Simulator::Schedule(m_transitDelay + m_transitFreq, &MultiLinkDevice::SwitchLink, this);
}



}   /* ns3 */

