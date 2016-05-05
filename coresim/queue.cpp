#include <climits>
#include <iostream>
#include <stdlib.h>
#include "assert.h"

#include "queue.h"
#include "packet.h"
#include "event.h"
#include "debug.h"

#include "../run/params.h"

extern double get_current_time(); // TODOm
extern void add_to_event_queue(Event* ev);
extern uint32_t dead_packets;
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

    if (params.ddc != 0) {
        if (location == 0) {
            this->propagation_delay = 10e-9;
        }
        else if (location == 1 || location == 2) {
            this->propagation_delay = 400e-9;
        }
        else if (location == 3) {
            this->propagation_delay = 210e-9;
        }
        else {
            assert(false);
        }
    }
    else {
        this->propagation_delay = params.propagation_delay;
    }
    this->p_arrivals = 0; this->p_departures = 0;
    this->b_arrivals = 0; this->b_departures = 0;

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

    if (location != 0 && packet->type == NORMAL_PACKET) {
        dead_packets += 1;
    }

    if(debug_flow(packet->flow->id))
        std::cout << get_current_time() << " pkt drop. flow:" << packet->flow->id
            << " type:" << packet->type << " seq:" << packet->seq_no
            << " at queue id:" << this->id << " loc:" << this->location << "\n";

    delete packet;
}

double Queue::get_transmission_delay(uint32_t size) {
    return size * 8.0 / rate;
}

void Queue::preempt_current_transmission() {
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

