#ifndef FP_TOPO_H
#define FP_TOPO_H

#include "../coresim/topology.h"
#include "fastpasshost.h"

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

class CutThroughFastpassTopology : public CutThroughTopology, public FastpassTopology {
public:
    CutThroughFastpassTopology(
                uint32_t num_hosts,
                uint32_t num_agg_switches,
                uint32_t num_core_switches,
                double bandwidth,
                uint32_t queue_type
                );

    virtual Queue* get_next_hop(Packet* p, Queue* q);
    virtual double get_oracle_fct(Flow* f);
};
#endif
