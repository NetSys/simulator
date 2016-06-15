#include "phostflow.h"
#include "phost.h"

#include "../run/params.h"

/* Implementation for pHost Flow */

extern void add_to_event_queue(Event* ev);
extern double get_current_time();
extern DCExpParams params;
extern uint32_t duplicated_packets_received;

PHostFlow::PHostFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d)
    : Flow(id, start_time, size, s, d) {
    this->remaining_packets = size_in_pkt;

    this->capabilities_sent = 0;
    this->next_seq_no = 0;
    this->sent_ack = false;

    this->timed_out = false;
    this->timeout_seqno = 0;
}

void PHostFlow::start_flow() {
    // send RTS
    Packet* p = new RTSCTS(true, get_current_time(), this, params.hdr_size, src, dst);
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), p, this->src->queue));
}

void PHostFlow::receive(Packet* p) {
    PHostToken* t;
    switch (p->type) {
        // sender side
        case CAPABILITY_PACKET:
            if (id == 2) std::cout << get_current_time() << " " << id << " " << p->seq_no <<  " got capa " << get_current_time() + params.capability_timeout * params.get_full_pkt_tran_delay() << "\n";
            t = new PHostToken(this, p->seq_no, get_current_time() + params.capability_timeout * params.get_full_pkt_tran_delay());
            ((PHost*) src)->received_capabilities.push(t);

            if (((PHost*) src)->host_proc_event == NULL || ((PHost*) src)->host_proc_event->cancelled) {
                ((PHost*) src)->host_proc_event = new HostProcessingEvent(get_current_time(), ((PHost*) src));
                add_to_event_queue(((PHost*) src)->host_proc_event);
            }
            break;
        case ACK_PACKET:
            // flow finished
            finished = true;
            finish_time = get_current_time();
            add_to_event_queue(new FlowFinishedEvent(get_current_time(), this));
            break;

        // receiver side
        case RTS_PACKET:
            if (id == 2) std::cout << get_current_time() << " " << id << " get RTS\n";
            ((PHost*) dst)->start_receiving(this);
            break;
        case NORMAL_PACKET:
            if (id == 2) std::cout << get_current_time() << " " << id << " " << p->seq_no <<  " got pkt\n";
            receive_data_pkt(p);
            break;

        default:
            assert(false);
            break;
    }

    delete p;
}

void PHostFlow::receive_data_pkt(Packet* p) {
    timed_out = false;
    timeout_seqno = 0;

    uint32_t seq = p->seq_no;
    bool found = false;
    for (auto it = window.begin(); it != window.end(); it++) {
        if (*it == seq) {
            window.erase(it);
            found = true;
            break;
        }
    }

    if (!found) {
        //duplicated packet
        duplicated_packets_received += 1;
        return;
    }

    remaining_packets--;

    // send ack if done
    if (remaining_packets == 0) {
        assert(window.size() == 0);
        this->retx_event->cancelled = true;
        this->retx_event = NULL;

        sent_ack = true;
        add_to_event_queue(
            new PacketQueuingEvent(
                get_current_time(), 
                new PlainAck(this, 0, params.hdr_size, this->dst, this->src), 
                this->dst->queue
            )
        );
    } else if (this->retx_event != NULL) {
        this->retx_event->cancelled = true;
        this->retx_event = NULL;
        ((PHost*) dst)->active_receiving_flows.push(this);
        
        if (((PHost*) dst)->capa_proc_evt == NULL || ((PHost*) dst)->capa_proc_evt->cancelled) {
            ((PHost*) dst)->capa_proc_evt = new PHostTokenProcessingEvent(get_current_time(), ((PHost*) dst));
            add_to_event_queue(((PHost*) dst)->capa_proc_evt);
        }
    }

}

// timeouts are receiver side
void PHostFlow::set_timeout(double time) {
    time = get_current_time() + params.capability_window_timeout * params.get_full_pkt_tran_delay();
    if (retx_event == NULL || retx_event->cancelled) {
        retx_event = new RetxTimeoutEvent(time, this);
        add_to_event_queue(retx_event);
    }
}

void PHostFlow::handle_timeout() {
    if (id == 2) std::cout << get_current_time() << " " << id << " timeout " << remaining_packets << " " << window.size() << "\n";
    assert(!sent_ack);
    this->timed_out = true;
    ((PHost*) dst)->active_receiving_flows.push(this);

    if (((PHost*) dst)->capa_proc_evt == NULL || ((PHost*) dst)->capa_proc_evt->cancelled) {
        ((PHost*) dst)->capa_proc_evt = new PHostTokenProcessingEvent(get_current_time(), ((PHost*) dst));
        add_to_event_queue(((PHost*) dst)->capa_proc_evt);
    }
}
