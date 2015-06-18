#include "../coresim/event.h"
#include "../coresim/packet.h"

#include "magicflow.h"
#include "magichost.h"

#include "../run/params.h"

extern DCExpParams params;
extern double get_current_time();
extern void add_to_event_queue(Event*);

MagicFlow::MagicFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d)
    : FountainFlowWithSchedulingHost(id, start_time, size, s, d) {
        this->send_count = 0;
        this->remaining_pkt_this_round = 0;
        this->ack_timeout = 0;
        this->virtual_rts_send_count = 0;
        if (params.magic_inflate == 1) {
            this->added_infl_time = false;
        }
        else {
            this->added_infl_time = true;
        }
    }

void MagicFlow::start_flow() {
    if (!this->added_infl_time) {
        add_to_event_queue(new FlowArrivalEvent(get_current_time() + 1.6e-6, this));
        this->added_infl_time = true;
        return;
    }
    else {
        FountainFlowWithSchedulingHost::start_flow();
    }
}

double MagicFlow::get_propa_time(){
    //return this->src->id/16 == this->dst->id/16?0.0000016:0.0000026;
    return 0.0000026;
}

Packet* MagicFlow::send(uint32_t seq) {
    uint32_t priority = 1;
    if(this->size_in_pkt > 8)
        priority = 2;
    //priority = this->remaining_pkt();
    Packet *p = new Packet(get_current_time(), this, seq, priority, mss + hdr_size, src, dst);
    total_pkt_sent++;
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), p, src->queue));
    return p;
}

void MagicFlow::send_pending_data() {
    Packet *p = this->send(next_seq_no);
    next_seq_no += mss;
    this->send_count++;
    assert(this->remaining_pkt_this_round > 0);
    this->remaining_pkt_this_round--;

    if(((SchedulingHost*) src)->host_proc_event == NULL || ((MagicHost*) src)->is_host_proc_event_a_timeout){
        if(((SchedulingHost*) src)->host_proc_event)
            ((SchedulingHost*) src)->host_proc_event->cancelled = true;
        double td = src->queue->get_transmission_delay(p->size);
        ((SchedulingHost*) src)->host_proc_event = new HostProcessingEvent(get_current_time() + td, (SchedulingHost*) src);
        add_to_event_queue(((SchedulingHost*) src)->host_proc_event);
    }
}

void MagicFlow::receive(Packet *p) {
    if (this->finished) {
        return;
    }
    if (p->type == NORMAL_PACKET) {
        received_count++;
        received_bytes += (p->size - hdr_size);
        //only send one ack per bdp
        //        if (received_count == size_in_pkt){
        //            assert( this == ((QuickSchedulingHost*)(this->dst))->flow_receiving );
        //            ((QuickSchedulingHost*)(this->dst))->flow_receiving = NULL;
        //        }

        if (received_count >= goal) {
            send_ack();
        }
    }
    else if (p->type == ACK_PACKET) {
        //assert(this == ((QuickSchedulingHost*)(this->src))->flow_sending );
        this->finished = true;
        //((QuickSchedulingHost*)(this->src))->flow_sending = NULL;
        //((QuickSchedulingHost*)(this->src))->schedule();
        add_to_event_queue(new FlowFinishedEvent(get_current_time(), this));
    }
    delete p;
}

double MagicFlow::estimated_finish_time() {
    if(this->finished || received_count >= size_in_pkt)
        return get_current_time();
    else
        return get_current_time() + (size_in_pkt - received_count) * 0.0000013;
}

unsigned MagicFlow::remaining_pkt() {
    return (uint)std::max((int)size_in_pkt - (int)received_count, (int)0);
}

