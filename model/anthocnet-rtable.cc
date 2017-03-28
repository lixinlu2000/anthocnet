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

#define NS_LOG_APPEND_CONTEXT                                   \
  if (ipv4) { std::clog << "[node " << std::setfill('0') << std::setw(2) \
    << ipv4->GetObject<Node> ()->GetId () +1 << "] "; }


#include "anthocnet-rtable.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("AntHocNetRoutingTable");
namespace ahn {


RoutingTableEntry::RoutingTableEntry() {
    this->pheromone = 0;
    this->virtual_pheromone = 0;
    
}

RoutingTableEntry::~RoutingTableEntry() {}


// ------------------------------------------------------
DestinationInfo::DestinationInfo() :
  expires_in(Simulator::Now()),
  no_broadcast_time(Seconds(0)),
  session_time(Seconds(0)),
  session_active(false)
  {}

DestinationInfo::~DestinationInfo() {
}


// --------------------------------------------------------
NeighborInfo::NeighborInfo() :
  expires_in(Simulator::Now()),
  avr_T_send(Seconds(0))
  {}

NeighborInfo::~NeighborInfo() {
}



RoutingTable::RoutingTable(Ptr<AntHocNetConfig> config, Ptr<Ipv4> ipv4) :
  ipv4(ipv4),

  config(config)
  {}
  
RoutingTable::~RoutingTable() {
  
}

void RoutingTable::SetConfig(Ptr<AntHocNetConfig> config) {
  this->config = config;
}

Ptr<AntHocNetConfig> RoutingTable::GetConfig() const {
  return this->config;
}


void RoutingTable::AddNeighbor(Ipv4Address nb) {
  if (!this->IsNeighbor(nb)) {
    this->nbs.insert(std::make_pair(nb, NeighborInfo()));
    this->AddDestination(nb);
  }
}

bool RoutingTable::IsNeighbor(Ipv4Address nb) {
  return (this->nbs.find(nb) != this->nbs.end());
}

bool RoutingTable::IsNeighbor(NbIt nb_it) {
  return (nb_it != this->nbs.end());
}

void RoutingTable::RemoveNeighbor(uint32_t iface_index, Ipv4Address address) {
  for (dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    this->RemovePheromone(dst_it->first, nb);
  }
  this->nbs.erase(nb);
}



void RoutingTable::AddDestination(Ipv4Address dst) {
    if (!this->IsDestination(dst)) {
    this->dsts.insert(std::make_pair(dst, DestinationInfo()));

    // TODO: Add initialized pheromone tables here?
    
  }
}

bool RoutingTable::IsDestination(Ipv4Address dst) {
  return (this->dsts.find(dst) != this->dsts.end());
}

bool RoutingTable::IsDestination(DstIt dst_it) {
  return (dst_it != this->dsts.end());
}

void RoutingTable::RemoveDestination(Ipv4Address dst) {
  
  for (nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    this->RemovePheromone(dst, nb_it->first);
  }
  this->dsts.erase(dst);
  
  // TODO: Remove also as neighbor
  
}

void RoutingTable::AddPheromone(Ipv4Address dst, Ipv4Address nb, 
                                double pher, double virt_pher) {
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  if (p_it == this->rtable.end()) {
    if (!this->IsDestination(dst) || !this->IsNeighbor(nb))
      return;
    
    p_it = this->rtable.insert(std::make_pair(dst, nb));
    
  }
  
  p_it->second.pheromne = pher;
  p_it->second.virtual_pheromone = virt_pher;
  
}

void RoutingTable::RemovePheromone(Ipv4Address dst, Ipv4Address nb) {
  this->rtable.erase(std::make_pair(dst, nb));
}

bool RoutingTable::HasPheromone(Ipv4Address dst, Ipv4Address nb bool virt) {
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  if (p_it == this->rtable.end())
    return false;
  
  if (!virt) {
    if (p_it->second.pheromone > this->config->min_pheromone)
      return true;
    else
      return false;
  } else {
    if (p_it->second.virtual_pheromone > this->config->min_pheromone)
      return true;
    else
      return false;
  }
  
}

void RoutingTable::SetPheromone(Ipv4Address dst, Ipv4Address nb, bool virt) {
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  
  if (p_it == this->rtable.end()) {
    this->AddPheromone(dst, nb, 0, 0);
  
  if (!virt)
    p_it->second.pheromone = pher;
  else
    p_it->seconds.virtual_pheromone = virt_pher;
}

double RoutingTable::GetPheromone(Ipv4Address dst, Ipv4Address nb, bool virt) {
  
  auto p_it = this->rtable.find(std::make_pair(dst, nb));
  
  if (p_it == this->rtable.end())
    return 0;
  
  if (!virt)
    return p_it->second.pheromone;
  else
    return p_it->seconds.virtual_pheromone;
}


void RoutingTable::UpdatePheromone(Ipv4Address dst, Ipv4Address nb, 
                                   double update, bool virt) {
  
  auto target_nb_it = this->nbs.find(nb);
  if (target_nb_it == this->nbs.end()) {
    NS_LOG_FUNCTION(this << "nb not found" << nb);
    return;
  }
  
  // If destination not exists, create new
  // and set initial pheromone
  auto dst_it = this->dsts.find(dst);
  if (dst_it == this->dsts.end()) {
      this->AddDestination(dst);
      this->AddPheromone(dst, 0, 0);
  }
  
  // Check if dst exists but now valid entry for the pair
  auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
  if (p_it == this->rtable.end()) {
    this->AddPheromone(dst, 0, 0);
  }
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    double old_phero = this->GetPheromone(dst, nb_it->first, virt);
    // This is the neigbor we are processing our data for
    if (nb_it == target_nb_it) {
      
      double new_phero = this->IncressPheromone(old_phero, update);
      this->SetPheromone(dst, nb_it->first, new_phero, virt);
    } else {
      
      // Evaporate the value
      double new_phero = this->EvaporatePheromone(old_phero);
      this->SetPheromone(dst, nb_it->first, new_phero, virt);
    } 
  }
}

void RoutingTable::RegisterSession(Ipv4Address dst) {
  auto dst_it = this->dsts.find(dst);
  if (this->IsDestination(dst_it)) {
    dst_it->second.session_time = Simulator::Now();
    dst_it->second.session_active = true;
  }
}

std::list<Ipv4Address> RoutingTable::GetSessions() {
  
  std::list<Ipv4Address> ret;
  for (auto dst_it = this->dsts.begin(); 
       dst_it != this->dsts.end(); ++dst_it){
    
    if (dst_it->second.session_active &&
        Simulator::Now() - dst_it->second.session_time 
          < this->config->session_expire){
      
      ret.push_back(dst_it->first);
    
    }
    else {
      dst_it->second.session_active = false;
    }
  }
  
  return ret;
}

bool RoutingTable::IsBroadcastAllowed(Ipv4Address dst) {
  
  // Check if destination exists
  auto dst_it = this->dsts.find(dst);
  if (!this->IsDestination(dst_it)) {
    this->AddDestination(dst);
    return false;
  }
  
  if (Simulator::Now() <= dst_it->second.no_broadcast_time) {
    NS_LOG_FUNCTION(this << "no bcast to" 
      << dst << " for " 
      << (dst_it->second.no_broadcast_time - Simulator::Now()).GetMilliSeconds());
    return false;
  }
  
  return true;
}

void RoutingTable::ProcessAck(Ipv4Address nb, Time last_hello) {
  
  // If we get an ack without a Hello, what do do?
  // Be happy or suspicios?
  
  auto nb_it = this->nbs.find(nb);
  if (this->IsNeighbor(nb_it)) {
    return;
  }
  
  Time delta = Simulator::Now() - last_hello;
  Time avr = nb_it->second.avr_T_send;
  
  if (avr == Seconds(0)) {
    nb_it->second.avr_T_send = delta;
  }
  else {
    nb_it->second.avr_T_send = 
      NanoSeconds(this->config->eta_value * avr.GetNanoSeconds()) +
      NanoSeconds((1.0 - this->config->eta_value) * delta.GetNanoSeconds());
  }
  
  NS_LOG_FUNCTION(this << "nb" << nb << 
    << "new avr_T_send" << nb_it->second.avr_T_send.GetMicroSeconds());
  
}

Time RoutingTable::GetTSend(Ipv4Address nb, uint32_t iface) {
  
  auto nb_it = this->nbs.find(nb);
  if (this->IsNeighbor(nb_it)) {
    return Seconds(0);
  }
  
  return nb_it->second.avr_T_send;
}

void RoutingTable::NoBroadcast(Ipv4Address dst, Time duration) {
  
  auto dst_it = this->dsts.find(dst);
  if (this->IsDestination(dst_it)) {
    this->AddDestination(dst);
    dst_it = this->dsts.find(dst);
  }
  
  dst_it->second.no_broadcast_time = Simulator::Now() + duration;
  
  return;
}

void RoutingTable::UpdateNeighbor(Ipv4Address nb) {
  
  auto nb_it = this->nbs.find(nb);
  if (!this->IsNeighbor(nb_it))
    return;
  
  nb_it->second.last_active = Simulator::Now();
  
  
}

// TODO: Remove Interval parameter, use config interval thing instead??
// TODO: Handle ret table as refenerence for speadup
std::set<Ipv4Address> RoutingTable::Update(Time interval) {
  
  
  
  std::set<Ipv4Address> ret;
  
  for (dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    if (nb_it->second.last_active + interval) < Simulator::Now()) {
      NS_LOG_FUNCTION(this << "dst" << nb_it->first << "timed out");
      this->RemoveDestination(dst_it->first);
    }
  }
  
  
  for (nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    if (nb_it->second.last_active + interval) < Simulator::Now()) {
      NS_LOG_FUNCTION(this << "nb" << nb_it->first << "timed out");
      
      ret.insert(nb_it->first);
      
    }
  }
  
  for (p_it = this->rtable.begin(); p_it != this->rtable.end(); ++p_it) {
    // NOTE: If use time based evaportation, use it here
  }
  
  return ret;
}

bool RoutingTable::SelectRoute(Ipv4Address dst, double beta,
                               Ipv4Address& nb,  Ptr<UniformRandomVariable> vr,
                               bool virt){
  
  
  // Check if destination is a neighbor
  auto temp_nb_it = this->nbs.find(dst);
  if (temp_nb_it != this->nbs.end()) {
    NS_LOG_FUNCTION(this << "dst" << dst << "is nb" << nb 
     << "usevirt" << consider_virtual);
    return true;
  }
  
  
  //Get the destination index:
  auto dst_it = this->dsts.find(dst);
  
  // Fail, if there are no entries to that destination at all
  if (dst_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "dst does not exist" << dst);
    return false;
  }
  
  double total_pheromone = this->SumPropability(dst, beta, consider_virtual)
  
  // NOTE: Very low pheromone can lead to the total_pheromone 
  // beeing rounded down to Zero. (When used with a high power) 
  // This leads to the system acting as if
  // there is no pheromone at all. This is most likely not an intended
  // behaviour and there should be a case to handle that
  
  // Fail, if there are no initialized entries (same as no entires at all)
  if (total_pheromone < this->config->min_pheromone) {
    NS_LOG_FUNCTION(this << "no initialized nbs");
    return false;
  }
  
  // To select with right probability, a random uniform variable between 
  // 0 and 1 is generated, then it iterates over the neighbors, calculates their
  // probability and adds it to an aggregator. If the aggregator gets over the 
  // random value, the particular Neighbor is selected.
  double select = vr->GetValue(0.0, 1.0);
  double selected = 0.0;
  
  NS_LOG_FUNCTION(this << "total_pheromone" << total_pheromone);
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    
    if (p_it == this->rtable.end())
      continue;
    
    // TODO: Consider virtual_malus
    if (virt && p_it->second.virtual_pheromone > p_it->second.pheromone) {
      selected += pow(p_it->second.virtual_pheromone, beta)/ total_pheromone);
    } else {
      selected += pow(p_it->second.pheromone, beta)/ total_pheromone);
    }
    
    if (selected > select) {
      nb = nb_it->first;
      break;
    }
    
  }
  
  // Never come here
  NS_LOG_FUNCTION(this << "never come here");
  return false;
}


bool RoutingTable::SelectRandomRoute(Ipv4Address& nb,
                                     Ptr<UniformRandomVariable> vr) {
  
  if (this->nbs.size() == 0) {
    return false;
  }
  
  uint32_t select = vr->GetInteger(0, this->nbs.size());
  uint32_t counter = 0;
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    if (counter == select) {
      nb = nb_it->first;
      NS_LOG_FUNCTION(this <<  "nb" << nb);
      return true;
    }
    
    counter++;
  }
  
  // Never come here
  return false;
}

void RoutingTable::ProcessNeighborTimeout(LinkFailureHeader& msg, 
                                          Ipv4Address nb) {
  
  for (auto dst_it = this->dsts.begin(); dst_it != this->dsts.end(); ++dst_it) {
    
    auto p_it = this->rtable.find(std::make_pair(dst_it->first, nb));
    if (p_it == this->rtable.end())
      continue;
    
    auto other_inits = this->IsOnly(dst_it->first, address, iface);
    
    if (!other_inits.first) {
      msg.AppendUpdate(dst_it->first, ONLY_VALUE, 0.0);
    }
    else if (other_inits.second < brk_pheromone) {
      msg.AppendUpdate(dst_it->first, NEW_BEST_VALUE, other_inits.second);
    }
    else {
     msg.AppendUpdate(dst_it->first, VALUE, 0.0); 
    }
  }
  NS_LOG_FUNCTION(this << "NB Timeout: " << msg);
  
  // After constructing the message, we can remove the neighbor
  this->RemoveNeighbor(iface, address);
  
  return;*/
}

void RoutingTable::ProcessLinkFailureMsg (LinkFailureHeader& msg,
                                          LinkFailureHeader& response,
                                          Ipv4Address origin){
  
  NS_ASSERT(msg.GetSrc() == origin);
  NS_ASSERT(msg.HasUpdates());
  
  NS_LOG_FUNCTION(this << "Processing::" << msg);
  
  // Need to check if we really have this neighbor as neighbor
  // If not just skip, since we do not have routes over this
  // node anyway
  if (!this->IsDestination(origin)) {
    NS_LOG_FUNCTION(this << origin << "not neighbor");
    return; 
  }
  
  // Now we evaluate the update list from this message
  while (msg.HasUpdates()) {
    
    linkfailure_list_t l = msg.GetNextUpdate();
    
    // Skip the destinations, we do not know about
    // since we do not have information that could be outdated anyway
    if (!this->IsDestination(l.dst))
      continue;
    
    // Check for alternatives
    auto other_inits = this->IsOnly(dst_it->first, origin);
    
    double old_phero = 0.0;
    double new_phero = 0.0;
    
    switch (l.status) {
      case VALUE:
        
        // DO nothing ???
        
        break;
      case ONLY_VALUE:
        
        // The route via linknb to dst is now broken
        // If it was broken before, no need to update
        if (!this->HasPheromone(l.dst, origin, false))
          continue;
        
        if (!other_inits.first) {
          // No alternatives, full breakage
          response.AppendUpdate(l.dst, ONLY_VALUE, 0.0);
        }
        else if (this->GetPheromone(l.dst, nb, false) > other_inits.second){
          // Best pheromone become invalid, send new one
          response.AppendUpdate(l.dst, NEW_BEST_VALUE, other_inits.second);
        }
        
        this->RemovePheromone(l.dst, origin, false);
        
        break;
        
      case NEW_BEST_VALUE:
        
        /*
        // If the route reported to have a new best value did not have any value
        // before, can bootstrap it, but we must not publish our 
        // found, since it leads
        // to an infinite message loop
        if (this->rtable[dst_index][linkif_index].pheromone !=
          this->rtable[dst_index][linkif_index].pheromone) {
          new_phero = this->Bootstrap(l.dst, origin, iface, l.new_phero, false);
          continue;
        }
        
        old_phero = this->rtable[dst_index][linkif_index].pheromone;
        
        // Use Bootstrap algorithm
        new_phero = this->Bootstrap(l.dst, origin, iface, l.new_phero, false);
        
        // If the updates pheromone was not the best to begin with, there is no need to 
        // inform the other nodes
        if (old_phero < other_inits.second) {
          continue;
        }
        
        if (new_phero < other_inits.second) {
          response.AppendUpdate(l.dst, NEW_BEST_VALUE, other_inits.second);
        }
        */
        break;
      default:
        break;
      
    }
    
  }
  
  NS_LOG_FUNCTION(this << "Response::" << response);
  return;
  */
}


void RoutingTable::ConstructHelloMsg(HelloMsgHeader& msg, uint32_t num_dsts, 
                                     Ptr<UniformRandomVariable> vr) {
  
  bool use_random = true;
  // If there are less known destinations than requested,
  // there is no need to select some randomly
  if (this->dsts.size() <= num_dsts) {
    use_random = false;
  }
  
  std::list<std::pair<Ipv4Address, double> > selection;
  
  for (auto dst_it = this->dsts.beggin(); dst_it != this->dsts.end(); ++dst_it) [
    
    Ipv4Address temp_dst = dst_it->first;
    double best_phero = 0.0
    
    for (auto nb_it != this->nbs.end(); ++nb_it) {
      
      auto p_it = this->rtable.find(std::make_pair(dst_it->first, nb_it->first));
      if (p_it == this->rtable.end())
        continue;
      
      if (std::abs(best_phero) > p_it->second.pheromone)
        best_phero = p_it->second.pheromone;
      
      
      if (std::abs(best_phero) > p_it->second.virtual_pheromone)
        best_phero = -1.0 * p_it->second.virtual_pheromone;
    }
   
   if (best_phero > this->config->min_pheromone)
     selection.push_back(std::make_pair(temp_dst, best_phero));
  }
  
  // Now select some of the pairs we found
  for (uint32_t i = 0; i < num_dsts; i++) {
    
    if (selection.empty()) 
      break;
    
    uint32_t select;
    if (use_random) {
      select = std::floor(vr->GetValue(0.0, selection.size()));
    }
    else {
      select = 0;
    }
    
    //NS_LOG_FUNCTION(this << "select" << select << "ndst" << selection.size());
    
    // Get to the selection
    auto sel_it = selection.begin();
    for (uint32_t c = select; c > 0; c--) {
      sel_it++;
    }
    
    NS_LOG_FUNCTION(this << "message" << msg);
    msg.PushDiffusion(sel_it->first, sel_it->second);
    selection.erase(sel_it); 
  }
}


void RoutingTable::HandleHelloMsg(HelloMsgHeader& msg) {
  
  if(!msg.IsValid()) {
    NS_LOG_FUNCTION(this << "Malformed HelloMsg -> dropped");
    return;
  }
  
  if (!this->IsNeighbor(msg.GetSrc()))
    this->AddNeighbor(msg.GetSrc());
  
  // Bootstrap information for every possible destination
  while (msg.GetSize() != 0) {
    
    auto diff_val = msg.PopDiffusion();
    
    // Get destination or add if not yet exist
    if (!this->IsDestination(diff_val.first))
      this->AddDestination(diff_val.first);
    
    bool is_virt = false;
    double bs_phero = diff_val.second;
    if (bs_phero < 0) {
      bs_phero *= -1;
      is_virt = true;
    }
    
    double T_id = this->GetTSend(diff_val.first).GetMilliSeconds();
    double new_phero = this->Bootstrap(bs_phero, T_id);
    
    // TODO: Add special case where real pheromone is used
    
    this->UpdatePheromone(diff_val.first, msg.GetSrc(), new_phero, true);
  }
}
/*

bool RoutingTable::ProcessBackwardAnt(Ipv4Address dst, uint32_t iface,
  Ipv4Address nb, uint64_t T_sd, uint32_t hops) {
    
  
  NS_LOG_FUNCTION(this << "dst" << dst << "iface" 
    << iface << "nb" << nb << "T_sd" << T_sd << "hops" << hops);
  // First search the destination and add it if it did not exist.
   // Check if destination already exists
  auto dst_it = this->dsts.find(dst);
  if (dst_it == this->dsts.end()) {
    this->AddDestination(dst);
    dst_it = this->dsts.find(dst);
  }
  
  // Find the neighbors iterators
  auto nb_it = this->dsts.find(nb);
  if (nb_it == this->dsts.end()) {
    NS_LOG_FUNCTION(this << "nb not in reach -> Ant dropped");
    return false;
  }
  
  auto nbif_it = nb_it->second.nbs.find(iface);
  if (nbif_it == nb_it->second.nbs.end()) {
    NS_LOG_FUNCTION(this << "interface not found -> Ant dropped");
    return false;
  }
  
  // Since both, the Neighbor and the Destination are found active,
  // reset their expiration dates.
  nb_it->second.expires_in = this->config->nb_expire;
  dst_it->second.expires_in = this->config->dst_expire;
  
  
  // Get the indexes into the pheromone of dst and nb table
  uint32_t nb_index = nbif_it->second.index;
  uint32_t dst_index = dst_it->second.index;
  
  // NOTE: This is the cost function.
  // One could get crazy here and have some 
  // really cool functions
  
  double T_id = (( ((double)T_sd / 1000000) + hops * this->config->T_hop) / 2);
  
  // Update the routing table
  RoutingTableEntry* ra = &this->rtable[dst_index][nb_index];
  
  // Check if hop count value is NAN
  if (ra->avr_hops != ra->avr_hops) {
    ra->avr_hops = hops;
  }
  else {
    ra->avr_hops = this->config->alpha_pheromone*ra->avr_hops +
      (1.0 - this->config->alpha_pheromone) * hops;
  }
  
  // Check if pheromone value is NAN
  if (ra->pheromone != ra->pheromone) {
    ra->pheromone = (1.0 / T_id);
  }
  else {
    ra->pheromone = 
      this->config->gamma_pheromone*ra->pheromone +
      (1.0 - this->config->gamma_pheromone) * (1.0 / T_id);
  }
  
  //this->UpdatePheromone(dst, nb, iface, T_id, hops);
  
  NS_LOG_FUNCTION(this << "updated pheromone" << ra->pheromone 
    << "average hops" << ra->avr_hops
    << "for" << dst_it->first << nb_it->first);
  
  return true;
}
*/



double RoutingTable::Bootstrap(double ph_value, double update) {
  return 1.0/(1.0/(update) + ph_value);
}


std::pair<bool, double> RoutingTable::IsOnly(Ipv4Address dst, 
                                             Ipv4Address nb) {
  
  
  bool other_inits = false;
  double best_phero = 0;
  auto marked_nb_it = this->nbs.find(nb);
  
  for(auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    if (nb_it == marked_nb_it)
      continue;
    
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    if (p_it == this->rtable.end())
      continue
    
    if (p_it->second.pheromone > this->config->min_pheromone) {
      other_inits = true;
      
      if(p_it->second.pheromone > best_phero) {
        best_phero = p_it->second.pheromone;
      }
      
    }
    
  }
  
  
  NS_ASSERT((other_inits && best_pheromone != 0) || (!other_inits));
  return std::make_pair(other_inits, best_pheromone);
}


double RoutingTable::SumPropability(Ipv4Address dst, double beta, bool virt) {
  
  double Sum = 0;
  
  for (auto nb_it = this->nbs.begin(); nb_it != this->nbs.end(); ++nb_it) {
    
    auto p_it = this->rtable.find(std::make_pair(dst, nb_it->first));
    
    if (p_it == this->rtable.end())
      continue;
    
    // TODO: Add consideration value
    if (virt) {
      if (p_it->second.virtual_pheromone > p_it->second.pheromone)
        Sum += pow(p_it->second.virtual_pheromone, beta);
      else
        Sum += pow(p_it->second.pheromone, beta);
    }
    else {
      Sum += pow(p_it->second.pheromone, beta);
    }
    
  }
  
}

double RoutingTable::EvaporatePheromone(doube ph_value) {
  return ph_value - (1- this->config->alpha) * ph_value;
}

double RoutingTable::IncressPheromone(double ph_value, double update) {
  return (this->config->gamma * ph_value + (1 - this->config->gamma) + update);
}

/*void RoutingTable::Print(Ptr<OutputStreamWrapper> stream) const {
  this->Print(*stream->GetStream());
}

// FIXME: This function causes program to crash (when used in NS_LOG_FUNCTION)
void RoutingTable::Print(std::ostream& os) const{
  
  for (auto dst_it1 = this->dsts.begin();
       dst_it1 != this->dsts.end(); ++dst_it1) {
    
    // Output the destination info 
    //os << " DST:[" << dst_it1->first << " : " << dst_it1->second.index ;
    os << " DST:[" << dst_it1->first;
    
    
    if (dst_it1->second.nbs.size() != 0) {
      
      os << " NB:( ";
      
      for (auto nb_it = dst_it1->second.nbs.begin();
           nb_it != dst_it1->second.nbs.end(); ++nb_it) {
      
        os << nb_it->first << " ";
      }
      os << ")";
    }
    os << "]";
  }
  
  os << std::endl;
  
  // Print the pheromone table
  for (auto dst_it1 = this->dsts.begin();
       dst_it1 != this->dsts.end(); ++dst_it1) {
    
    os << dst_it1->first << ":";
    
    // Iterate over all neigbors
    for (auto dst_it2 = this->dsts.begin();
         dst_it2 != this->dsts.end(); ++dst_it2) {
      
      for (auto nb_it = dst_it2->second.nbs.begin();
           nb_it != dst_it2->second.nbs.end(); ++nb_it) {
        
        uint32_t dst_idx = dst_it1->second.index;
        uint32_t nb_idx = nb_it->second.index;
        
        os << "(" << dst_it2->first << ":" << nb_it->first << "):";
        os << this->rtable[dst_idx][nb_idx].pheromone << "|";  
        os << this->rtable[dst_idx][nb_idx].avr_hops << "|";
        os << this->rtable[dst_idx][nb_idx].virtual_pheromone;
        os << "->(" << dst_idx << ":" << nb_idx << ")\t";
        }
    }
    
    os << std::endl;
    
  }
  
}

std::ostream& operator<< (std::ostream& os, RoutingTable const& t) {
  t.Print(os);
  return os;
}*/

}
}