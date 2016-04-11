#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <cstddef>
#include <iostream>
#include <math.h>
#include <vector>

#include "node.h"
#include "assert.h"
#include "packet.h"
#include "queue.h"

#include "../ext/factory.h"

#include "../run/params.h"

class Topology {
    public:
        Topology();
        virtual Queue *get_next_hop(Packet *p, Queue *q) = 0;
        virtual double get_oracle_fct(Flow* f) = 0;

        uint32_t num_hosts;

        std::vector<Host *> hosts;
        std::vector<Switch*> switches;
};

class PFabricTopology : public Topology {
    public:
        PFabricTopology(
                uint32_t num_hosts, 
                uint32_t num_agg_switches,
                uint32_t num_core_switches, 
                double bandwidth, 
                uint32_t queue_type
                );

        virtual Queue* get_next_hop(Packet *p, Queue *q);
        virtual double get_oracle_fct(Flow* f);

        uint32_t num_agg_switches;
        uint32_t num_core_switches;

        std::vector<AggSwitch*> agg_switches;
        std::vector<CoreSwitch*> core_switches;
};


class BigSwitchTopology : public Topology {
    public:
        BigSwitchTopology(uint32_t num_hosts, double bandwidth, uint32_t queue_type);
        virtual Queue *get_next_hop(Packet *p, Queue *q);
        virtual double get_oracle_fct(Flow* f);

        CoreSwitch* the_switch;
};

#endif
