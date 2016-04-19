#include "ideal.h"

#include <algorithm>
#include "assert.h"

extern DCExpParams params;

extern double get_current_time();
extern void add_to_event_queue(Event *);
extern uint32_t num_outstanding_packets;
extern IdealArbiter* ideal_arbiter;

IdealArbiter::IdealArbiter() {
    active_flows = new std::vector<IdealFlow*>();
    srcs = new std::array<bool, 144>();
    dsts = new std::array<bool, 144>();

    assert(srcs->size() == params.num_hosts);
}

bool compareFlows(IdealFlow* i, IdealFlow* j) {
    uint32_t i_remain = i->acked - i->size;
    uint32_t j_remain = j->acked - j->size;
    return i_remain > j_remain;
}

void IdealArbiter::flow_arrival(IdealFlow* f) {
    for (auto it = active_flows->begin(); it != active_flows->end(); it++) {
        assert(*it != f);
    }

    active_flows->push_back(f);
    std::stable_sort(active_flows->begin(), active_flows->end(), compareFlows);

    compute_schedule();
}

void IdealArbiter::flow_finished(IdealFlow* f) {
    assert(f->sent >= f->size);
    ((IdealHost*) f->src)->active_flow = NULL;
    for (auto it = active_flows->begin(); it != active_flows->end(); it++) {
        if (*it == f) {
            active_flows->erase(it);
            compute_schedule();
            return;
        }
    }
}

void IdealArbiter::compute_schedule() {
    srcs->fill(false);
    dsts->fill(false);

    for (auto it = active_flows->begin(); it != active_flows->end(); it++) {
        IdealFlow* f = (IdealFlow*) *it;
        if (f->finished) {
            if (((IdealHost*) f->src)->active_flow == f) {
                ((IdealHost*) f->src)->active_flow = NULL;
            }
            it = active_flows->erase(it);
            if (it == active_flows->end()) {
                break;
            }
        }
        if (!srcs->at(f->src->id) && !dsts->at(f->dst->id)) {
            auto previous = ((IdealHost*) f->src)->active_flow;
            if (previous != NULL && 
                previous->id != f->id && 
                ((IdealHost*) f->src)->host_proc_event != NULL
            ) {
                // new flow
                ((IdealHost*) f->src)->host_proc_event->cancelled = true;
                ((IdealHost*) f->src)->host_proc_event = NULL;
            }
            ((IdealHost*) f->src)->active_flow = f;

            srcs->at(f->src->id) = true;
            dsts->at(f->dst->id) = true;
                
            if (((IdealHost*) f->src)->host_proc_event == NULL || ((IdealHost*) f->src)->host_proc_event->cancelled) {
                ((IdealHost*) f->src)->host_proc_event = new HostProcessingEvent(get_current_time(), (IdealHost*) f->src);
                add_to_event_queue(((IdealHost*) f->src)->host_proc_event);
            }
        }
    }
}

IdealHost::IdealHost(uint32_t id, double rate, uint32_t queue_type) : SchedulingHost(id, rate, queue_type) {
    this->dispatch = ideal_arbiter;
    this->active_flow = NULL;
}

void IdealHost::start(Flow* f) {
    dispatch->flow_arrival((IdealFlow*) f);
}

void IdealHost::send() {
    // send packets from arbiter-assigned flow
    assert(host_proc_event == NULL);
    double td = this->queue->get_transmission_delay(params.mss + params.hdr_size);
    if (active_flow != NULL && !active_flow->finished) {
        assert(active_flow->src == this);

        host_proc_event = new HostProcessingEvent(get_current_time() + td + INFINITESIMAL_TIME, this);
        add_to_event_queue(host_proc_event);
        
        active_flow->send_pending_data();
    }
}

IdealAck::IdealAck(Flow* flow, uint32_t recv, Host* src, Host* dst) : PlainAck(flow, 0, params.hdr_size, src, dst) {
    received = recv;
}

IdealFlow::IdealFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d) : Flow(id, start_time, size, s, d) {
    received = 0;
    acked = 0;
    sent = 0;
}

void IdealFlow::start_flow() {
    ((IdealHost*) src)->start(this);
}

void IdealFlow::handle_timeout() {
    this->retx_event = NULL;
    if (!finished) {
        this->start_flow();
    }
}

void IdealFlow::send_pending_data() {
    if (finished) {
        cancel_retx_event();
        return;
    }

    // send a packet
    send(0);
    sent += mss;

    if (sent == size) { // if there was a timeout (sent > size), just wait for ACK
        assert(retx_event == NULL);
        set_timeout(get_current_time() + params.retx_timeout_value);
        
        ((IdealHost*) src)->host_proc_event->cancelled = true;
        ((IdealHost*) src)->dispatch->flow_finished(this);
    }
}

void IdealFlow::receive(Packet* p) {
    if (this->finished) {
        delete p;
        return;
    }

    Packet* ack;
    switch (p->type) {
        case NORMAL_PACKET:
            this->received += mss;
            ack = new IdealAck(this, this->received, this->dst, this->src);
            add_to_event_queue(new PacketQueuingEvent(get_current_time(), ack, this->dst->queue));
            break;
        case ACK_PACKET:
            if (((IdealAck*) p)->received < this->acked) {
                break; // old ack
            }

            this->acked = ((IdealAck*) p)->received;
            num_outstanding_packets -= 1;

            if (this->acked >= size) {
                assert(finished == false);
                
                if (retx_event != NULL) {
                    if (id == 21) std::cout << get_current_time() * 1e6 << " 21 finished??\n";
                    cancel_retx_event();
                }

                if (sent > size) {
                    if (((IdealHost*) src)->host_proc_event != NULL) {
                        ((IdealHost*) src)->host_proc_event->cancelled = true;
                    }
                    ((IdealHost*) src)->dispatch->flow_finished(this);
                }

                if (((IdealHost*) src)->active_flow == this) {
                    ((IdealHost*) src)->active_flow = NULL;
                }

                finished = true;
                finish_time = get_current_time();
                flow_completion_time = finish_time - start_time;
                FlowFinishedEvent *ev = new FlowFinishedEvent(get_current_time(), this);
                add_to_event_queue(ev);
            }
            break;
        default:
            assert(false);
    }

    delete p;
}

uint32_t IdealFlow::get_priority(uint32_t seq) {
    return size - acked;
}
