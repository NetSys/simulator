#include "murphy_topology.h"
#include "../run/params.h"

extern DCExpParams params;

MurphyTopology::MurphyTopology() : Topology() {
    h1 = new Host(0, params.bandwidth, 1, 1);
    h2 = new Host(1, params.bandwidth, 1, 1);
    s = new CoreSwitch(2, 2, params.bandwidth, 1);

    //connect queues together
    h1->queue->set_src_dst(h1, s);
    h2->queue->set_src_dst(h2, s);

    s->queues[0]->set_src_dst(s, h1);
    s->queues[1]->set_src_dst(s, h2);
}

Queue* MurphyTopology::get_next_hop(Packet* p, Queue* q) {
    if (q->src->id == 0) {
        assert(q->dst->id == 2 && p->src->id == 0);
        return s->queues[1];
    }
    else if (q->src->id == 1) {
        assert(q->dst->id == 2 && p->src->id == 1);
        return s->queues[0];
    }
    else if (q->src->id == 2) {
        return NULL;
    }
}

double MurphyTopology::get_oracle_fct(Flow* f) {
    return 1.0;
}

