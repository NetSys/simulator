#ifndef TCP_FLOW_H
#define TCP_FLOW_H

#include "../coresim/flow.h"
#include "../coresim/node.h"
#include "../coresim/packet.h"

class PacedFlow : public Flow {
    public:
        PacedFlow(uint32_t id, double start_time, uint32_t size, Host* s, Host* d);
        virtual void start_flow();
        virtual void send_pending_data();
        virtual void receive(Packet *p);
        virtual uint32_t get_priority(uint32_t seq);

        uint32_t counter;
};

#endif
