/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef MULTI_LINK_DEVICE_H
#define MULTI_LINK_DEVICE_H

#include "ns3/wifi-net-device.h"
#include "ns3/wifi-helper.h"
#include "ns3/object.h"
#include "ns3/socket.h"
#include "ns3/data-rate.h"

namespace ns3 {

class MultiLinkDevice : public Object
{
public:
    static TypeId GetTypeId (void);

    MultiLinkDevice();
    virtual ~MultiLinkDevice();
    
    /* create MLD STA1  */
    void SetSTA1(const NetDeviceContainer device);
    /* get MLD STA1 */
    Ptr<WifiNetDevice> GetSTA1();
    /* create MLD STA2  */
    void SetSTA2(const NetDeviceContainer device);
    /* get MLD STA2 */
    Ptr<WifiNetDevice> GetSTA2();
    /* get address of STA1 */
    Address GetAddress1();
    /* get address of STA2 */
    Address GetAddress2();
    /* show how many packet have been sent */
    uint32_t GetTotalByte();
    /* clear the transiting state */
    void Clear();  
    /* transition delay setting */
    void SetTransitDelay(Time delay);
    /* get transition delay value */
    Time GetTransitDelay();
    /* transition frequency setting */
    void SetTransitFreq(Time freq);
    /* get transition frequency value */
    Time GetTransitFreq();
    /* get total sending error number */
    uint32_t GetSendError();
    /* create and bind socket */
    void SocketSetting(Ptr<Socket> socket1, Ptr<Socket> socket2, Address addr1, Address addr2, DataRate cbrRate, bool isAP); 


    /* schedule the next packet transmission */
    void SchduleNextTx();
    /* use STAs socket to send packets*/
    void SendPacket ();
    /* switch from one eMLSR link to another eMLSR link */
    void SwitchLink();

private:
    uint32_t    m_totalByte;       // total packet number that have been sent
    uint32_t    m_totalReceive;    // total packet number that have been received
    uint32_t    m_linkNumber;      // the eMLSR link that transmitting now
    Time        m_transitFreq;     // transition frequency (MicroSeconds)
    Time        m_transitDelay;    // transition delay (MicroSeconds)
    bool        m_isTransit;       // whether this device is trasiting 
    uint32_t    m_SendError;       // total sending error number
    DataRate    m_cbrRate;         // rate the data is generated
    uint32_t    m_packetSize;      //  packet size
    uint32_t    m_residualBits;    // number of generated, but not sent, bits
    Time        m_lastStartTime;   // time last packet sent
    Ptr<Packet> m_unsentPacket;    // unsent packet cached for future attempt
    bool        m_isAP;            // see this device is AP or not

    Ptr<Socket> m_socket1;       // socket of STA1
    Ptr<Socket> m_socket2;       // socket of STA2
    Ptr<WifiNetDevice> m_sta1;
    Ptr<WifiNetDevice> m_sta2;
};

}   /* ns3 */

#endif /* MULTI_LINK_DEVICE_H */

