#include "../coresim/packet.h"
#include "../coresim/queue.h"

#include "fastpassTopology.h"

#include "../run/params.h"

extern DCExpParams params;

FastpassTopology::FastpassTopology(
        uint32_t num_hosts,
        uint32_t num_agg_switches,
        uint32_t num_core_switches,
        double bandwidth,
        uint32_t queue_type
        ) : PFabricTopology(num_hosts, num_agg_switches, num_core_switches, bandwidth, queue_type) {
    uint32_t hosts_per_agg_switch = num_hosts / num_agg_switches;
    double c1 = bandwidth;
    double c2 = hosts_per_agg_switch * bandwidth / num_core_switches;
    this->arbiter = new FastpassArbiter(num_hosts, c1, queue_type);

    //
    // make sure we only replace one switch with the fastpass switch
    //
    assert(switches[0] == agg_switches[0]);

    AggSwitch* as = agg_switches[0];
    std::vector<Queue*> queues = as->queues;
    auto new_as = new FastpassAggSwitch(0, hosts_per_agg_switch, c1, num_core_switches, c2, queue_type);
    new_as->queues = queues;

    new_as->queue_to_arbiter = Factory::get_queue(num_agg_switches + num_core_switches, c1, params.queue_size, queue_type, 0, 3);
    arbiter->queue->set_src_dst(arbiter, new_as);
    new_as->queue_to_arbiter->set_src_dst(new_as, arbiter);

    agg_switches[0] = new_as;
    switches[0] = new_as;
    delete as;
}

Queue* FastpassTopology::get_next_hop(Packet* p, Queue* q) {
    if (q->src->type == HOST) {
        if ((p->src->host_type == FASTPASS_ARBITER && p->dst->id / 16 == 0) || (p->dst->host_type == FASTPASS_ARBITER && p->src->id / 16 == 0)) {
            return ((Switch*) q->dst)->queues[p->dst->id % 16];
        }
    }
    else if (q->src->type == SWITCH) {
        if (((Switch*) q->src)->switch_type == AGG_SWITCH) {
            if (p->dst->host_type == FASTPASS_ARBITER) return ((Switch*) q->dst)->queues[0];
        }
        if (((Switch*) q->src)->switch_type == CORE_SWITCH) {
            if (p->dst->host_type == FASTPASS_ARBITER) {
                assert(((Switch*) q->dst)->id == 0);
                return ((FastpassAggSwitch*) q->dst)->queue_to_arbiter;
            }
        }
    }

    return PFabricTopology::get_next_hop(p, q);
}


CutThroughFastpassTopology::CutThroughFastpassTopology(
        uint32_t num_hosts,
        uint32_t num_agg_switches,
        uint32_t num_core_switches,
        double bandwidth,
        uint32_t queue_type
        ) : PFabricTopology(num_hosts, num_agg_switches, num_core_switches, bandwidth, queue_type),
    CutThroughTopology(num_hosts, num_agg_switches, num_core_switches, bandwidth, queue_type), 
    FastpassTopology(num_hosts, num_agg_switches, num_core_switches, bandwidth, queue_type)  {}

Queue* CutThroughFastpassTopology::get_next_hop(Packet* p, Queue* q) {
    return FastpassTopology::get_next_hop(p, q);
}

double CutThroughFastpassTopology::get_oracle_fct(Flow* f) {
    return CutThroughTopology::get_oracle_fct(f);
}

