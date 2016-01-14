#ifndef DCTCP_FLOW_H
#define DCTCP_FLOW_H

#include "dctcpPacket.h"
#include "../coresim/flow.h"
#include "../coresim/node.h"

class DctcpFlow : public Flow {
    public:
        DctcpFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);
        // subclass to set priorities to 1
        virtual uint32_t get_priority(uint32_t seq);

        // Receiver Side
        virtual void receive_data_pkt(Packet* p);
        //void mark_receipt(Packet* p);
        //void determine_ack(DctcpPacket* p);
       
        // After looking at DCTCP NS2 code, Delayed ACK is not used.
        //
        // ECN-Echo + Delayed ACK state machine
        //
        // while no ECN set, send 1 ACK per m packets with ECN = 0
        // if ECN set in received packet, immediately send ACK with ECN = 0
        // while ECN = 1, send 1 ACK per m packets with ECN = 1
        // if ECN not set in received packet, immediately send ACK with ECN = 1
        // delayed ACK m number typically = 2. params.dctcp_delayed_ack_freq

        //bool ce_state;
        //uint32_t delayed_ack_counter;

        // Sender Side
        virtual void receive(Packet* p);
        void receive_ack(Ack* a);
        
        // sender response to ECN-Echo: cwnd <- cwnd * (1 - a/2)
        // a <- (1 - g) * a + g * F
        // where g = params.dctcp_discount_rate and F = fraction of packets marked in last window
        virtual void increase_cwnd();
        std::deque<bool>* ecn_history;
        double dctcp_alpha;
        double dctcp_g;
};

#endif
