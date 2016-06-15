#ifndef PHOST_FLOW_H
#define PHOST_FLOW_H

#include "../coresim/flow.h"
#include "../coresim/node.h"

class PHostFlow : public Flow {
    public:
        PHostFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);
        virtual void start_flow();
        virtual void receive(Packet* p);

        virtual void set_timeout(double time);
        virtual void handle_timeout();

        //sender side
        uint32_t remaining_packets;

        //receiver side
        virtual void receive_data_pkt(Packet* p);
        std::vector<uint32_t> window;
        uint32_t next_seq_no;
        uint32_t capabilities_sent;
        bool sent_ack;
        
        bool timed_out;
        uint32_t timeout_seqno;
};

#endif
