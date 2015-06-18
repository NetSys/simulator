#ifndef FP_TOPO_H
#define FP_TOPO_H

#include "../coresim/topology.h"

class FastpassArbiter;

class FastpassTopology : public PFabricTopology {
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
