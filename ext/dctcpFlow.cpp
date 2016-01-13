#include <cmath>
#include <assert.h>
#include "dctcpFlow.h"

#include "../coresim/event.h"
#include "../run/params.h"

extern double get_current_time(); 
extern void add_to_event_queue(Event *);
extern int get_event_queue_size();
extern DCExpParams params;
extern uint32_t num_outstanding_packets;
extern uint32_t duplicated_packets_received;

DctcpFlow::DctcpFlow(
    uint32_t id, 
    double start_time, 
    uint32_t size, 
    Host *s, 
    Host *d
    ) : Flow(id, start_time, size, s, d) {
    //ce_state = false;
    //delayed_ack_counter = 0;
    dctcp_g = 0.0625;
    dctcp_alpha = 0;
    ecn_history = new std::deque<bool>(max_cwnd);
}

void DctcpFlow::receive(Packet* p) {
    if (finished) {
        delete p;
        return;
    }

    if (p->type == ACK_PACKET) {
        Ack *a = (Ack *) p;
        receive_ack(a);
    }
    else if(p->type == NORMAL_PACKET) {
        this->receive_data_pkt(p);
    }
    else {
        assert(false);
    }

    delete p;
}

//Receiver Side

//Delayed Ack Implementation
/*
void DctcpFlow::mark_receipt(Packet* p) {
    received_count++;
    total_queuing_time += p->total_queuing_delay;

    if (received.count(p->seq_no) == 0) {
        received[p->seq_no] = true;
        if(num_outstanding_packets >= ((p->size - hdr_size) / (mss)))
            num_outstanding_packets -= ((p->size - hdr_size) / (mss));
        else
            num_outstanding_packets = 0;
        received_bytes += (p->size - hdr_size);
    } else {
        duplicated_packets_received += 1;
    }
    if (p->seq_no > max_seq_no_recv) {
        max_seq_no_recv = p->seq_no;
    }
}

void DctcpFlow::determine_ack(DctcpPacket* p) {
    // Determing which ack to send
    uint32_t s = recv_till;
    bool in_sequence = true;
    std::vector<uint32_t> sack_list;
    while (s <= max_seq_no_recv) {
        if (received.count(s) > 0) {
            if (in_sequence) {
                recv_till += mss;
            } else {
                sack_list.push_back(s);
            }
        } else {
            in_sequence = false;
        }
        s += mss;
    }

    //send_ack(recv_till, sack_list); // Cumulative Ack
    Packet* a = new DctcpAck(this, recv_till, sack_list, hdr_size, dst, src, p->ecn, delayed_ack_counter);
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), a, dst->queue));
}

void DctcpFlow::receive_data_pkt(Packet* p) {
    mark_receipt(p);

    DctcpPacket *dcp = (DctcpPacket*) p;
    if (ce_state) {
        if (dcp->ecn) {
            if (delayed_ack_counter == params.dctcp_delayed_ack_freq) {
                determine_ack(dcp);
                delayed_ack_counter = 0;
            }
            else {
                delayed_ack_counter++;
            }
        }
        else {
            determine_ack(dcp);
            delayed_ack_counter = 0;
            ce_state = false;
        }
    }
    else {
        if (dcp->ecn) {
            determine_ack(dcp);
            delayed_ack_counter = 0;
            ce_state = true;
        }
        else {
            if (delayed_ack_counter == params.dctcp_delayed_ack_freq) {
                determine_ack(dcp);
                delayed_ack_counter = 0;
            }
            else {
                delayed_ack_counter++;
            }
        }
    }
}
*/

void DctcpFlow::receive_data_pkt(Packet* p) {
    received_count++;
    total_queuing_time += p->total_queuing_delay;

    if (received.count(p->seq_no) == 0) {
        received[p->seq_no] = true;
        if(num_outstanding_packets >= ((p->size - hdr_size) / (mss)))
            num_outstanding_packets -= ((p->size - hdr_size) / (mss));
        else
            num_outstanding_packets = 0;
        received_bytes += (p->size - hdr_size);
    } else {
        duplicated_packets_received += 1;
    }
    if (p->seq_no > max_seq_no_recv) {
        max_seq_no_recv = p->seq_no;
    }
    // Determing which ack to send
    uint32_t s = recv_till;
    bool in_sequence = true;
    std::vector<uint32_t> sack_list;
    while (s <= max_seq_no_recv) {
        if (received.count(s) > 0) {
            if (in_sequence) {
                recv_till += mss;
            } else {
                sack_list.push_back(s);
            }
        } else {
            in_sequence = false;
        }
        s += mss;
    }

    Packet *a = new DctcpAck(this, recv_till, sack_list, hdr_size, dst, src, ((DctcpPacket*) p)->ecn); //Acks are dst->src
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), a, dst->queue));
}

void DctcpFlow::increase_cwnd() {
    // cwnd <- cwnd * (1 - a/2)
    // floor(x + 0.5) same as round to nearest integer
    cwnd_mss = (uint32_t) floor(cwnd_mss * (1 - dctcp_alpha / 2) + 0.5);
}

void DctcpFlow::receive_ack(Ack* a) {
    DctcpAck *dca = (DctcpAck*) a;
    uint32_t ack = a->seq_no;
    std::vector<uint32_t> sack_list = a->sack_list;

    this->scoreboard_sack_bytes = sack_list.size() * mss;

    // On timeouts; next_seq_no is updated to the last_unacked_seq;
    // In such cases, the ack can be greater than next_seq_no; update it
    if (next_seq_no < ack) {
        next_seq_no = ack;
    }

    // New ack!
    if (ack > last_unacked_seq) {
        // Update the last unacked seq
        last_unacked_seq = ack;
        
        // Update ecn_history
        //for (uint32_t i = 0; i < dca->delayed_num; i++) // only needed for delayed ack
        assert(dca->ecn == 1 || dca->ecn == 0);
        ecn_history->push_front(dca->ecn);
        while (ecn_history->size() > max_cwnd) 
            ecn_history->pop_back();

        // Update alpha
        // a <- (1 - g) * a + g * F
        uint32_t ecn_set_count = 0;
        uint32_t sz = ecn_history->size();
        for (uint32_t i = 0; i < cwnd_mss; i++)  {
            assert(i < sz);
            if (ecn_history->at(i) == true) {
                ecn_set_count ++;
            }

            // TODO doing the following causes an unexplainable bug at -O2 but not -Og
            //
            //volatile char setecn = (ecn_history->at(i) ) ? 1 : 0;
            //std::cout << setecn << " " << ecn_history->at(i) << "\n";
            //assert(ecn_history->at(i) == 0 || ecn_history->at(i) == 1);
            //assert(setecn == 1 || setecn == 0);
            //ecn_set_count += setecn;
        }
        assert(ecn_set_count <= cwnd_mss && cwnd_mss <= sz);
        double frac_ecn = ecn_set_count / cwnd_mss;
        assert(frac_ecn >= 0.0 && frac_ecn <= 1.0);
        dctcp_alpha = (1 - dctcp_g) * dctcp_alpha + dctcp_g * frac_ecn;
        assert(dctcp_alpha >= 0.0 && dctcp_alpha <= 1.0);

        // Adjust cwnd
        increase_cwnd();

        // Send the remaining data
        send_pending_data();

        // Update the retx timer
        if (retx_event != NULL) { // Try to move
            cancel_retx_event();
            if (last_unacked_seq < size) {
                // Move the timeout to last_unacked_seq
                double timeout = get_current_time() + retx_timeout;
                set_timeout(timeout);
            }
        }
    }

    if (ack == size && !finished) {
        finished = true;
        received.clear();
        finish_time = get_current_time();
        flow_completion_time = finish_time - start_time;
        FlowFinishedEvent *ev = new FlowFinishedEvent(get_current_time(), this);
        add_to_event_queue(ev);
    }
}

uint32_t DctcpFlow::get_priority(uint32_t seq) {
    return 1;
}
