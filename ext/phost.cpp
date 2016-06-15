#include "phost.h"

#include <assert.h>
#include <stdlib.h>

#include "../coresim/event.h"
#include "../coresim/flow.h"
#include "../coresim/packet.h"
#include "../coresim/debug.h"

#include "factory.h"

#include "../run/params.h"

extern double get_current_time();
extern void add_to_event_queue(Event*);
extern DCExpParams params;

PHostTokenProcessingEvent::PHostTokenProcessingEvent(double time, PHost *h)
    : Event(PHOST_TOKEN_PROCESSING_EVENT, time) {
        this->host = h;
    }

PHostTokenProcessingEvent::~PHostTokenProcessingEvent() {
    if (host->capa_proc_evt == this) {
        host->capa_proc_evt = NULL;
    }
}

void PHostTokenProcessingEvent::process_event() {
    this->host->capa_proc_evt = NULL;
    this->host->send_capability();
}

bool PHostFlowComparator::operator() (PHostToken* a, PHostToken* b){
    if (a->flow == b->flow)
        return a->seqno > b->seqno;
    if (params.deadline && params.schedule_by_deadline) 
        return a->flow->deadline > b->flow->deadline;
    if (a->flow->remaining_packets > b->flow->remaining_packets)
        return true;
    else if (a->flow->remaining_packets == b->flow->remaining_packets)
        return a->flow->start_time > b->flow->start_time;
    else
        return false;
}

bool PHostReceiverFlowComparator::operator() (PHostFlow* a, PHostFlow* b){
    if(params.deadline && params.schedule_by_deadline) {
        return a->deadline > b->deadline;
    }
    else {
        if ((a->size - a->capabilities_sent) > (b->size - b->capabilities_sent)) {
            return true;
        } else if ((a->size - a->capabilities_sent) == (b->size - b->capabilities_sent)) {
            return a->start_time > b->start_time;
        } else {
            return false;
        }
    }
}

PHost::PHost(uint32_t id, double rate, uint32_t queue_type) : SchedulingHost(id, rate, queue_type) {
    this->capa_proc_evt = NULL;
}

// sender side
void PHost::send() {
    PHostToken* t = NULL;
    double to = 0;

    while (to < get_current_time()) {
        if (received_capabilities.size() > 0) {
            t = received_capabilities.top();
            received_capabilities.pop();
            to = t->timeout;
        } 
        else {
            return;
        }
    }

    if (t != NULL) {
        assert(t->flow->src == this);

        Packet *p = new Packet(
                get_current_time(), 
                t->flow, 
                t->seqno, 
                1, 
                params.mss + params.hdr_size, 
                t->flow->src, 
                t->flow->dst
                );
        t->flow->total_pkt_sent++;

        add_to_event_queue(new PacketQueuingEvent(get_current_time(), p, this->queue));
    }

    if (this->host_proc_event == NULL || this->host_proc_event->cancelled) {
        this->host_proc_event = new HostProcessingEvent(
                get_current_time() 
                + params.get_full_pkt_tran_delay(), 
                this
            );
        add_to_event_queue(this->host_proc_event);
    }
}

//receiver side
void PHost::start_receiving(Flow* f) {
    active_receiving_flows.push((PHostFlow*) f);

    if (capa_proc_evt == NULL || capa_proc_evt->cancelled) {
        capa_proc_evt = new PHostTokenProcessingEvent(get_current_time(), this);
        add_to_event_queue(capa_proc_evt);
    }
}

void PHost::send_capability() {
    Packet* capa = NULL;
    PHostFlow* f = NULL;
    std::vector<PHostFlow*> passed_over;
    while(!active_receiving_flows.empty()) {
        f = active_receiving_flows.top();
        if (f->sent_ack) {
            active_receiving_flows.pop();
            continue;
        }
        if (f->timed_out && f->window.size() >= params.capability_window) {
            assert(!f->window.empty());
            // timed out, retx old packets
            capa = new Packet(get_current_time(), f, f->window[f->timeout_seqno], 0, params.hdr_size, f->dst, f->src);
            capa->type = CAPABILITY_PACKET;
            f->timeout_seqno = (f->timeout_seqno + 1) % f->window.size();
        }
        else if (f->remaining_packets <= f->window.size() || f->window.size() >= params.capability_window || f->window.size() >= f->size_in_pkt) {
            // wait for a timeout or a received packet
            f->set_timeout(0);
        }
        else if (f->capabilities_sent <= f->size_in_pkt) {
            capa = new Packet(get_current_time(), f, f->next_seq_no, 0, params.hdr_size, f->dst, f->src);
            capa->type = CAPABILITY_PACKET;
            f->window.push_back(f->next_seq_no);
            f->next_seq_no += params.mss;
            f->capabilities_sent++;
        }

        if (capa != NULL) {
            f->set_timeout(0);
            add_to_event_queue(new PacketQueuingEvent(get_current_time(), capa, this->queue));
            
            capa_proc_evt = new PHostTokenProcessingEvent(get_current_time() + params.get_full_pkt_tran_delay(), this);
            add_to_event_queue(capa_proc_evt);
            break;
        } 
        else {
            active_receiving_flows.pop();
            passed_over.push_back(f);
        }
    }

    while(!passed_over.empty()) {
        PHostFlow* f = passed_over.back();
        passed_over.pop_back();
        active_receiving_flows.push(f);
    }
}
