#ifndef MURPHY_TOPOLOGY_H
#define MURPHY_TOPOLOGY_H

#include <cstddef>
#include <iostream>
#include <math.h>
#include <vector>
#include "assert.h"

#include "../coresim/node.h"
#include "../coresim/packet.h"
#include "../coresim/queue.h"
#include "../coresim/topology.h"

class MurphyTopology : public Topology {
    public:
        MurphyTopology();
        virtual Queue* get_next_hop(Packet* p, Queue* q);
        virtual double get_oracle_fct(Flow* f);

        Host* h1;
        Switch* s;
        Host* h2;
};
#endif
