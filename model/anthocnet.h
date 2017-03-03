/*
 * Copyright (c) 2016 Leon Tan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef ANTHOCNET_H
#define ANTHOCNET_H

#include "anthocnet-rtable.h"
#include "anthocnet-packet.h"
#include "anthocnet-pcache.h"

#include <list>

#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/arp-cache.h"
#include "ns3/traced-callback.h"
#include "ns3/wifi-phy.h"

#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"


// TODO: Make these settable by GetTypeId()
//       instead of hardcoding them
#define MAX_INTERFACES 30
#define ANTHOCNET_PORT 5555

namespace ns3 {
namespace ahn {
  
class RoutingProtocol : public Ipv4RoutingProtocol {
public:
  
  
  //ctor
  RoutingProtocol();
  //dtor
  ~RoutingProtocol();
  
  static TypeId GetTypeId (void);
  static const uint32_t AODV_PORT;
  
  // Inherited from Ipv4RoutingProtocol
  // Look up their documentation in ns3 reference manual
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  virtual void DoDispose();
  
protected:
    virtual void DoInitialize();
  
private:
  
  // All the utiliy and callback functions of the protocol go here
  // Sets up the operation of the protocol
  void Start();
  
  Ptr<Socket> FindSocketWithInterfaceAddress
    (Ipv4InterfaceAddress addr) const;
  
  Ptr<Ipv4Route> LoopbackRoute(const Ipv4Header& hdr, 
    Ptr<NetDevice> oif) const;
  
  /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);
    
  /**
   * \brief Finds the index of the Socket, used in the rtable.
   * \param socket The pointer to the sockeT;
   * \returns The index into the socket table.
   */  
  uint32_t FindSocketIndex(Ptr<Socket>)const;
  
  void StartForwardAnt(Ipv4Address dst);
  
  TracedCallback<Ptr<Packet const>, std::string, Ipv4Address> ant_drop;
  TracedCallback<Ptr<Packet const>, std::string, Ipv4Address> data_drop;
  
  void UnicastForwardAnt(uint32_t iface, Ipv4Address dst, ForwardAntHeader ant);
  void UnicastBackwardAnt(uint32_t iface, Ipv4Address dst, BackwardAntHeader ant);
  void BroadcastForwardAnt(Ipv4Address dst);
  void BroadcastForwardAnt(Ipv4Address dst, ForwardAntHeader ant);
  
  void SendCachedData(Ipv4Address dst);
  
  // Add ARP cache to be used to allow layer 2 notifications processing
  void AddArpCache (Ptr<ArpCache>);
  // Don't use given ARP cache any more (interface is down)
  void DelArpCache (Ptr<ArpCache>);
  
  // Search all interfaces arpcaches for that mac address
  std::vector<Ipv4Address> LookupMacAddress (Mac48Address addr);
  
  // Holds pointers to all arp caches, such that it can
  // look up IP addresses from MACs. Needed for L2 support
  std::vector<Ptr<ArpCache> > arp_cache;
  
  // ----------------------------------------------
  // Called when there is an error in the Layer 2 link
  void ProcessTxError (WifiMacHeader const& header);
  
  // Called after receving a packet at layer 2 (Needed as metric)
  void ProcessPhyRxTrace(Ptr<Packet const> packet);
  void ProcessMacRxTrace(Ptr<Packet const> packet);
  
  // Used by RouteInput, is the other side to ProcessRxTrace
  void UpdateAvrTMac();
  
  void ProcessMonitorSnifferRx(Ptr<Packet const> packet, 
                              uint16_t frequency, uint16_t channel, 
                              uint32_t rate, WifiPreamble isShortPreable,
                              WifiTxVector tx_vector, mpduInfo mpdu,
                              signalNoiseDbm snr);
  
  // ----------------------------------------------
  // Callback function for receiving a packet
  void Recv(Ptr<Socket> socket);
  
  // Callback to do a deferred send
  void Send(Ptr<Socket>, Ptr<Packet>, Ipv4Address);
  
  // Send a HelloAnt every other second
  void HelloTimerExpire();
  
  // Update the RoutingTable (mainly throw out old data)
  void RTableTimerExpire();
  
  // -------------------------------------------------
  // Ant Handlers
  
  // Handles receiving of a HelloAnt
  void HandleHelloMsg(Ptr<Packet> packet, uint32_t iface);
  
  // Handles receiving of a ForwardAnt
  void HandleForwardAnt(Ptr<Packet> packet, uint32_t iface);
  
  // Handles receiving of a BackwardAnt
  void HandleBackwardAnt(Ptr<Packet> packet,Ipv4Address orig_src, uint32_t iface);
  
  //-----------------------------------------------
  // All the network config stuff go here 
  
  // The time to live of a new born forward and
  uint8_t initial_ttl;
  
  // -------------------------------------------------
  // Timers and their intervals
  
  // The frequency in which to send Hello Ants
  Time hello_interval;
  Timer hello_timer;
  
  // Frequency, in which to update the RoutingTable
  Time rtable_update_interval;
  Timer rtable_update_timer;
  
  // Frequency, in which proactive ants are sent out
  Time pr_ant_interval;
  Timer pr_ant_timer;
  
  
  // Time until an entry in the datacache expires and the packet is dropped
  Time dcache_expire;
  
  // Time until a neighbor expires
  Time nb_expire;
  
  // Time until a destination expires
  Time dst_expire;
  
  // Time after a RouteOutput that a session is considered active
  Time session_expire;
  
  // Drawback time after a broadcast
  Time no_broadcast;
  
  // The alpha value of the runinng average calculation
  double alpha_T_mac;
  
  // The T_hop heuristic
  double T_hop;
  
  // The decay factor in the calculation of the avergage hops
  double alpha_pheromone;
  
  // The decay factor for the calculation of the pheromone
  double gamma_pheromone;
  
  // The decay factor for the average Rx time calculation
  double eta_value;
  
  // These values are used by the snr part of the cost calculation
  // When snr is considered good or bad
  double snr_threshold;
  // The extra cost factor to a bad snr
  double bad_snr_cost;
  
  Ptr<UniformRandomVariable> uniform_random;
  
  //----------------------------------------------
  // All the global state of the protocol go here
  
  // Used to calculate avr_T_mac
  Time last_rx_begin;
  
  // Last time the Hello Timer expired
  Time last_hello;
  
  // The running average of the T_mac value
  // This is calculated by getting the Time difference
  // between last rx_begin and the start of routeinput
  Time avr_T_mac;
  
  // The last measured snr
  double last_snr; 
  
  // Holds the loopback device
  Ptr<NetDevice> lo;
  
  // The routing table
  RoutingTable rtable;
  
  // The Cache for the data that has no route yet
  PacketCache data_cache;
  
  // The IP protocol
  Ptr<Ipv4> ipv4;
  
  // Holds information about the interfaces
  Ptr<Socket> sockets[MAX_INTERFACES];
  
  std::map< Ptr<Socket>, Ipv4InterfaceAddress> socket_addresses;
  
};

// 
}
}

#endif /* ANTHOCNET_H */

