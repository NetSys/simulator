#ifndef FP_TOPO_H
#define FP_TOPO_H

#include "../coresim/topology.h"
#include "fastpasshost.h"

class FastpassAggSwitch : public AggSwitch {
    public:
        FastpassAggSwitch(uint32_t id, uint32_t nq1, double r1, uint32_t nq2, double r2, uint32_t queue_type);
        Queue* queue_to_arbiter;
};

class FastpassTopology : virtual public PFabricTopology {
    public:
        FastpassTopology(
                uint32_t num_hosts,
                uint32_t num_agg_switches,
                uint32_t num_core_switches,
                double bandwidth,
                uint32_t queue_type
                );
        virtual Queue* get_next_hop(Packet* p, Queue* q);

        FastpassArbiter* arbiter;
};

#endif
