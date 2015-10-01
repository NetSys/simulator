#include "fastpasshost.h"
#include "fastpassflow.h"

#include "../coresim/packet.h"
#include "../coresim/topology.h"
#include "../coresim/event.h"
#include "../coresim/debug.h"

#include "fastpassTopology.h"

extern Topology *topology;
extern double get_current_time();
extern void add_to_event_queue(Event*);
extern DCExpParams params;
extern uint32_t num_outstanding_packets;

FastpassFlow::FastpassFlow(
        uint32_t id, 
        double start_time, 
        uint32_t size,
        Host *s, 
        Host *d
        ) : Flow(id, start_time, size, s, d) {
    this->sender_remaining_num_pkts = this->size_in_pkt;
    this->arbiter_received_rts = false;
    this->arbiter_finished = false;
    this->sender_acked_count = 0;
    this->sender_acked_until = 0;
    this->sender_last_pkt_sent = -1;
    this->sender_finished = false;
}

void FastpassFlow::start_flow() {
    update_remaining_size();
}

void FastpassFlow::update_remaining_size() {
    FastpassRTS* rts = new FastpassRTS(this, this->src, dynamic_cast<FastpassTopology*>(topology)->arbiter, this->sender_finished?-1:this->sender_remaining_num_pkts);
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), rts, src->queue));
}

void FastpassFlow::send_ack_pkt(uint32_t seq) {
    PlainAck* ack = new PlainAck(this, seq, params.hdr_size, this->dst, this->src);
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), ack, this->dst->queue));
}

void FastpassFlow::send_schedule_pkt(FastpassEpochSchedule* schd) {
    FastpassSchedulePkt* pkt = new FastpassSchedulePkt(this, dynamic_cast<FastpassTopology*>(topology)->arbiter, this->src, schd);
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), pkt, dynamic_cast<FastpassTopology*>(topology)->arbiter->queue));
}


int FastpassFlow::next_pkt_to_send()
{
    int pkt = this->sender_last_pkt_sent;
    for(int i = 0; i < this->size_in_pkt; i++){
        pkt++;
        if(pkt >= this->size_in_pkt)
            pkt = sender_acked_until;
        if(sender_acked.count(pkt) == 0)
            return pkt;
    }
}

void FastpassFlow::send_data_pkt() {
    this->sender_last_pkt_sent = next_pkt_to_send();
    Packet *p = new Packet(get_current_time(), this, this->sender_last_pkt_sent * mss, 1, mss + hdr_size, src, dst);
    if(debug_flow(this->id))
        std::cout << get_current_time() << " flow " << this->id << " send data " << this->sender_last_pkt_sent << " \n";
    total_pkt_sent++;
    next_seq_no += mss;
    if(sender_remaining_num_pkts > 0) sender_remaining_num_pkts--;
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), p, src->queue));
    if(this->sender_remaining_num_pkts == 0)
        add_to_event_queue(new FastpassTimeoutEvent(get_current_time() + params.fastpass_epoch_time, this));
}


void FastpassFlow::fastpass_timeout()
{
    if(!this->sender_finished)
    {
        this->sender_remaining_num_pkts = this->size_in_pkt - this->sender_acked_count;
        this->update_remaining_size();
    }
}

void FastpassFlow::receive(Packet *p) {
    if (p->type == FASTPASS_RTS) {
        if(debug_flow(this->id))
            std::cout << get_current_time() << " flow " << this->id << " received rts\n";
        dynamic_cast<FastpassTopology*>(topology)->arbiter->receive_rts((FastpassRTS*) p);
    } else if (p->type == FASTPASS_SCHEDULE) {
        if(debug_flow(this->id))
            std::cout << get_current_time() << " flow " << this->id << " received schedule\n";
        ((FastpassHost*) this->src)->receive_schedule_pkt((FastpassSchedulePkt*) p);
    } else if (p->type == NORMAL_PACKET) {
        if(debug_flow(this->id))
            std::cout << get_current_time() << " flow " << this->id << " received data seq" << p->seq_no << "\n";
        this->send_ack_pkt(p->seq_no);
        this->received_bytes += mss;
        if(receiver_received.count(p->seq_no) == 0)
        {
            receiver_received.insert(p->seq_no);
            if(num_outstanding_packets >= ((p->size - hdr_size) / (mss)))
                num_outstanding_packets -= ((p->size - hdr_size) / (mss));
            else
                num_outstanding_packets = 0;
        }

    } else if (p->type == ACK_PACKET) {
        if(debug_flow(this->id))
            std::cout << get_current_time() << " flow " << this->id << " received ack seq" << p->seq_no << "\n";
        int acked_pkt = p->seq_no/mss;
        if(sender_acked.count(acked_pkt) == 0)
        {
            sender_acked.insert(acked_pkt);
            sender_acked_count++;
            while(sender_acked.count(sender_acked_until) > 0){
                sender_acked_until++;
            }
        }
        if(!this->sender_finished && sender_acked_count == this->size_in_pkt){
            this->sender_finished = true;
            this->update_remaining_size();
            add_to_event_queue(new FlowFinishedEvent(get_current_time(), this));
        }
    } else {
        assert(false);
    }
    delete p;
}

void FastpassFlow::schedule_send_pkt(double time) {
    add_to_event_queue(new FastpassFlowProcessingEvent(time, this));
}


ArbiterProcessingEvent::ArbiterProcessingEvent(double time, FastpassArbiter* a) : Event(ARBITER_PROCESSING, time) {
    this->arbiter = a;
}

ArbiterProcessingEvent::~ArbiterProcessingEvent() {
}

void ArbiterProcessingEvent::process_event() {
    this->arbiter->arbiter_proc_evt = NULL;
    this->arbiter->schedule_epoch();
}

