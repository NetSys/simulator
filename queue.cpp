#include "queue.h"
#include "packet.h"
#include "event.h"
#include "params.h"
#include <climits>
#include <iostream>
#include <stdlib.h>
#include "assert.h"
#include "debug.h"

extern double get_current_time(); // TODOm
extern void add_to_event_queue(Event *ev);
extern DCExpParams params;

uint32_t Queue::instance_count = 0;

/* Queues */
Queue::Queue(uint32_t id, double rate, uint32_t limit_bytes, int location) {
  this->id = id;
  this->unique_id = Queue::instance_count++;
  this->rate = rate; // in bps
  this->limit_bytes = limit_bytes;
  this->bytes_in_queue = 0;
  this->busy = false;
  this->queue_proc_event = NULL;
  //this->packet_propagation_event = NULL;
  this->location = location;

  this->propagation_delay = params.propagation_delay;
  this->p_arrivals = 0; this->p_departures = 0;
  this->b_arrivals = 0; this->b_departures = 0;

  this->dropss = 0; this->dropsl = 0; this->dropll = 0;
  this->pkt_drop = 0;
  this->spray_counter=std::rand();
  this->packet_transmitting = NULL;
}

void Queue::set_src_dst(Node *src, Node *dst) {
  this->src = src;
  this->dst = dst;
}

void Queue::enque(Packet *packet) {
  p_arrivals += 1;
  b_arrivals += packet->size;
  if (bytes_in_queue + packet->size <= limit_bytes) {
    packets.push_back(packet);
    bytes_in_queue += packet->size;
  } else {
    pkt_drop++;
    drop(packet);
  }
}

Packet *Queue::deque() {
  if (bytes_in_queue > 0) {
    Packet *p = packets.front();
    packets.pop_front();
    bytes_in_queue -= p->size;
    p_departures += 1;
    b_departures += p->size;
    return p;
  }
  return NULL;
}

void Queue::drop(Packet *packet) {
  packet->flow->pkt_drop++;
  if(packet->seq_no < packet->flow->size){
    packet->flow->data_pkt_drop++;
  }
  if(packet->type == ACK_PACKET)
    packet->flow->ack_pkt_drop++;

  if(debug_flow(packet->flow->id))
    std::cout << get_current_time() << " pkt drop. flow:" << packet->flow->id
        << " type:" << packet->type << " seq:" << packet->seq_no
        << " at queue id:" << this->id << " loc:" << this->location << "\n";

  delete packet;
}

double Queue::get_transmission_delay(uint32_t size) {
    return size * 8.0 / rate;
}

void Queue::preempt_current_transmission(){
  if(params.preemptive_queue && busy){
      this->queue_proc_event->cancelled = true;
      assert(this->packet_transmitting);

      uint delete_index;
      bool found = false;
      for (delete_index = 0; delete_index < packets.size(); delete_index++) {
        if (packets[delete_index] == this->packet_transmitting) {
          found = true;
          break;
        }
      }
      if(found){
        bytes_in_queue -= packet_transmitting->size;
        packets.erase(packets.begin() + delete_index);
      }

      for(uint i = 0; i < busy_events.size(); i++){
        busy_events[i]->cancelled = true;
      }
      busy_events.clear();
      //drop(packet_transmitting);//TODO: should be put back to queue
      enque(packet_transmitting);
      packet_transmitting = NULL;
      queue_proc_event = NULL;
      busy = false;
  }
}

/* PFabric Queue */
PFabricQueue::PFabricQueue(uint32_t id, double rate, uint32_t limit_bytes, int location)
 : Queue(id, rate, limit_bytes, location) {
}

void PFabricQueue::enque(Packet *packet) {
  p_arrivals += 1;
  b_arrivals += packet->size;
  packets.push_back(packet);
  bytes_in_queue += packet->size;
  packet->last_enque_time = get_current_time();
  if (bytes_in_queue > limit_bytes) {
    uint32_t worst_priority = 0;
    uint32_t worst_index = 0;
    for (uint32_t i = 0; i < packets.size(); i++) {
      if (packets[i]->pf_priority >= worst_priority) {
        worst_priority = packets[i]->pf_priority;
        worst_index = i;
      }
    }
    bytes_in_queue -= packets[worst_index]->size;
    Packet *worst_packet = packets[worst_index];

    packets.erase(packets.begin() + worst_index);
    pkt_drop++;
    drop(worst_packet);
  }
}


Packet* PFabricQueue::deque() {
  if (bytes_in_queue > 0) {

    uint32_t best_priority = UINT_MAX;
    Packet *best_packet = NULL;
    uint32_t best_index = 0;
    for (uint32_t i = 0; i < packets.size(); i++) {
      Packet* curr_pkt = packets[i];
      if (curr_pkt->pf_priority < best_priority) {
        best_priority = curr_pkt->pf_priority;
        best_packet = curr_pkt;
        best_index = i;
      }
    }

    for (uint32_t i = 0; i < packets.size(); i++) {
      Packet* curr_pkt = packets[i];
      if (curr_pkt->flow->id == best_packet->flow->id) {
        best_index = i;
        break;
      }
    }
    Packet *p = packets[best_index];
    bytes_in_queue -= p->size;
    packets.erase(packets.begin() + best_index);

    p_departures += 1;
    b_departures += p->size;

    p->total_queuing_delay += get_current_time() - p->last_enque_time;
    if(p->type ==  NORMAL_PACKET){
        if(p->flow->first_byte_send_time < 0)
            p->flow->first_byte_send_time = get_current_time();
        if(this->location == 0)
            p->flow->first_hop_departure++;
        if(this->location == 3)
            p->flow->last_hop_departure++;
    }
    return p;

  } else {
    return NULL;
  }
}


/* Implementation for probabilistically dropping queue */
ProbDropQueue::ProbDropQueue(uint32_t id, double rate, uint32_t limit_bytes,
  double drop_prob, int location)
  : Queue(id, rate, limit_bytes, location) {
  this->drop_prob = drop_prob;
}

void ProbDropQueue::enque(Packet *packet) {
  p_arrivals += 1;
  b_arrivals += packet->size;

  if (bytes_in_queue + packet->size <= limit_bytes) {
    double r = (1.0 * rand()) / (1.0 * RAND_MAX);
    if (r < drop_prob) {
      return;
    }
    packets.push_back(packet);
    bytes_in_queue += packet->size;
    if (!busy) {
      add_to_event_queue(new QueueProcessingEvent(get_current_time(), this));
      this->busy = true;
      //if(this->id == 7) std::cout << "!!!!!queue.cpp:189\n";
      this->packet_transmitting = packet;
    }
  }
}
