#include "fountainflow.h"

#include "../coresim/event.h"
#include "../coresim/packet.h"
#include "../coresim/debug.h"

#include "schedulinghost.h"

#include "../run/params.h"

extern double get_current_time();
extern void add_to_event_queue(Event*);
extern DCExpParams params;

FountainFlow::FountainFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d) : Flow(id, start_time, size, s, d) {
    this->goal = this->size_in_pkt;
}

void FountainFlow::send_pending_data() {
    if (this->finished) {
        return;
    }
    Packet *p = send(next_seq_no);
    next_seq_no += mss;
    //need to schedule next one
    double td = src->queue->get_transmission_delay(p->size);
    flow_proc_event = new FlowProcessingEvent(get_current_time() + td, this);
    add_to_event_queue(flow_proc_event);
}

Packet* FountainFlow::send(uint32_t seq) {
    uint32_t priority = 1;
    Packet *p = new Packet(get_current_time(), this, seq, priority, mss + hdr_size, src, dst);
    total_pkt_sent++;
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), p, src->queue));
    return p;
}

void FountainFlow::send_ack() {
    Packet *ack = new PlainAck(this, 0, hdr_size, dst, src);
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), ack, dst->queue));
}

void FountainFlow::receive(Packet *p) {
    if (this->finished) {
        return;
    }
    if (p->type == NORMAL_PACKET) {
        received_count++;
        //only send one ack per bdp
        if (received_count >= goal && (received_count - goal) % 7 == 0) {
            send_ack();
        }
    }
    else if (p->type == ACK_PACKET) {
        if (flow_proc_event != NULL) {
            flow_proc_event->cancelled = true;
        }
        add_to_event_queue(new FlowFinishedEvent(get_current_time(), this));
    }
    else if (p->type == RTS_PACKET){

    }
}


FountainFlowWithSchedulingHost::FountainFlowWithSchedulingHost(uint32_t id, double start_time, uint32_t size, Host *s, Host *d) : FountainFlow(id, start_time, size, s, d) {
}

void FountainFlowWithSchedulingHost::start_flow() {
    ((SchedulingHost*) this->src)->start(this);
}

void FountainFlowWithSchedulingHost::send_pending_data() {
    if (this->finished) {
        ((SchedulingHost*) this->src)->send();
        return;
    }

    Packet *p = this->send(next_seq_no);
    next_seq_no += mss;

    double td = src->queue->get_transmission_delay(p->size);
    ((SchedulingHost*) src)->host_proc_event = new HostProcessingEvent(get_current_time() + td, (SchedulingHost*) src);
    add_to_event_queue(((SchedulingHost*) src)->host_proc_event);    
}

void FountainFlowWithSchedulingHost::receive(Packet *p) {
    if (this->finished) {
        return;
    }
    if (p->type == NORMAL_PACKET) {
        received_count++;

        //only send one ack per bdp
        if (received_count >= goal && (received_count - goal) % 7 == 0) {
            send_ack();
        }
    }
    else if (p->type == ACK_PACKET) {
        add_to_event_queue(new FlowFinishedEvent(get_current_time(), this));
    }
}

