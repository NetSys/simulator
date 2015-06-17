#ifndef PFABRIC_FLOW_H
#define PFABRIC_FLOW_H

#include "../coresim/flow.h"
#include "../coresim/node.h"

class PFabricFlow : public Flow {
    public:
        PFabricFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);
        uint32_t ssthresh;
        uint32_t count_ack_additive_increase;
        virtual void increase_cwnd();
        virtual void handle_timeout();
};

#endif
