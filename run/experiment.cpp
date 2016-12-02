#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <deque>
#include <stdint.h>
#include <cstdlib>
#include <ctime>
#include <map>
#include <iomanip>
#include "assert.h"
#include "math.h"

#include "../coresim/flow.h"
#include "../coresim/packet.h"
#include "../coresim/node.h"
#include "../coresim/event.h"
#include "../coresim/topology.h"
#include "../coresim/queue.h"
#include "../coresim/random_variable.h"

#include "../ext/factory.h"
#include "../ext/fountainflow.h"
#include "../ext/capabilityflow.h"
#include "../ext/fastpassTopology.h"

#include "flow_generator.h"
#include "stats.h"
#include "params.h"

#include "../ext/ideal.h"

extern Topology *topology;
extern double current_time;
extern std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
extern std::deque<Flow*> flows_to_schedule;
extern std::deque<Event*> flow_arrivals;

extern uint32_t num_outstanding_packets;
extern uint32_t max_outstanding_packets;
extern DCExpParams params;
extern void add_to_event_queue(Event*);
extern void read_experiment_parameters(std::string conf_filename, uint32_t exp_type);
extern void read_flows_to_schedule(std::string filename, uint32_t num_lines, Topology *topo);
extern uint32_t duplicated_packets_received;

extern uint32_t num_outstanding_packets_at_50;
extern uint32_t num_outstanding_packets_at_100;
extern uint32_t arrival_packets_at_50;
extern uint32_t arrival_packets_at_100;

extern double start_time;
extern double get_current_time();

extern void run_scenario();

void validate_flow(Flow* f){
    double slowdown = 1000000.0 * f->flow_completion_time / topology->get_oracle_fct(f);
    if(slowdown < 0.999999){
        std::cout << "Flow " << f->id << " has slowdown " << slowdown << "\n";
        //assert(false);
    }
    //if(f->first_byte_send_time < 0 || f->first_byte_send_time < f->start_time - INFINITESIMAL_TIME)
    //    std::cout << "Flow " << f->id << " first_byte_send_time: " << f->first_byte_send_time << " start time:"
    //        << f->start_time << "\n";
}

void debug_flow_stats(std::deque<Flow *> flows){
    std::map<int,int> freq;
    for (uint32_t i = 0; i < flows.size(); i++) {
        Flow *f = flows[i];
        if(f->size_in_pkt == 3){
            int fct = (int)(1000000.0 * f->flow_completion_time);
            if(freq.find(fct) == freq.end())
                freq[fct] = 0;
            freq[fct]++;
        }
    }
    for(auto it = freq.begin(); it != freq.end(); it++)
        std::cout << it->first << " " << it->second << "\n";
}

void assign_flow_deadline(std::deque<Flow *> flows)
{
    ExponentialRandomVariable *nv_intarr = new ExponentialRandomVariable(params.avg_deadline);
    for(uint i = 0; i < flows.size(); i++)
    {
        Flow* f = flows[i];
        double rv = nv_intarr->value();
        f->deadline = f->start_time + std::max(topology->get_oracle_fct(f)/1000000.0 * 1.25, rv);
        //std::cout << f->start_time << " " << f->deadline << " " << topology->get_oracle_fct(f)/1000000 << " " << rv << "\n";
    }
}

void printQueueStatistics(Topology *topo) {
    double totalSentFromHosts = 0;

    uint64_t dropAt[4];
    uint64_t total_drop = 0;
    for (auto i = 0; i < 4; i++) {
        dropAt[i] = 0;
    }

    for (uint i = 0; i < topo->hosts.size(); i++) {
        int location = topo->hosts[i]->queue->location;
        dropAt[location] += topo->hosts[i]->queue->pkt_drop;
    }


    for (uint i = 0; i < topo->switches.size(); i++) {
        for (uint j = 0; j < topo->switches[i]->queues.size(); j++) {
            int location = topo->switches[i]->queues[j]->location;
            dropAt[location] += topo->switches[i]->queues[j]->pkt_drop;
        }
    }

    for (int i = 0; i < 4; i++) {
        total_drop += dropAt[i];
    }
    for (int i = 0; i < 4; i++) {
        std::cout << "Hop:" << i << " Drp:" << dropAt[i] << "("  << (int)((double)dropAt[i]/total_drop * 100) << "%) ";
    }

    for (auto h = (topo->hosts).begin(); h != (topo->hosts).end(); h++) {
        totalSentFromHosts += (*h)->queue->b_departures;
    }

    std::cout << " Overall:" << std::setprecision(2) <<(double)total_drop*1460/totalSentFromHosts << "\n";

    double totalSentToHosts = 0;
    for (auto tor = (topo->switches).begin(); tor != (topo->switches).end(); tor++) {
        for (auto q = ((*tor)->queues).begin(); q != ((*tor)->queues).end(); q++) {
            if ((*q)->rate == params.bandwidth) totalSentToHosts += (*q)->b_departures;
        }
    }

    double dead_bytes = totalSentFromHosts - totalSentToHosts;
    double total_bytes = 0;
    for (auto f = flows_to_schedule.begin(); f != flows_to_schedule.end(); f++) {
        total_bytes += (*f)->size;
    }

    double simulation_time = current_time - start_time;
    double utilization = (totalSentFromHosts * 8.0 / 144.0) / simulation_time;
    double dst_utilization = (totalSentToHosts * 8.0 / 144.0) / simulation_time;

    std::cout
        << "DeadPackets " << 100.0 * (dead_bytes/total_bytes)
        << "% DuplicatedPackets " << 100.0 * duplicated_packets_received * 1460.0 / total_bytes
        << "% Utilization " << utilization / 10000000000 * 100 << "% " << dst_utilization / 10000000000 * 100  
        << "%\n";
}

void run_experiment(int argc, char **argv, uint32_t exp_type) {
    if (argc < 3) {
        std::cout << "Usage: <exe> exp_type conf_file" << std::endl;
        return;
    }

    std::string conf_filename(argv[2]);
    read_experiment_parameters(conf_filename, exp_type);
    params.num_hosts = 144;
    params.num_agg_switches = 9;
    params.num_core_switches = 4;
    
    if (params.flow_type == FASTPASS_FLOW) {
        topology = new FastpassTopology(params.num_hosts, params.num_agg_switches, params.num_core_switches, params.bandwidth, params.queue_type);
    }
    else if (params.big_switch) {
        topology = new BigSwitchTopology(params.num_hosts, params.bandwidth, params.queue_type);
    } 
    else {
        topology = new PFabricTopology(params.num_hosts, params.num_agg_switches, params.num_core_switches, params.bandwidth, params.queue_type);
    }

    uint32_t num_flows = params.num_flows_to_run;

    FlowGenerator *fg;
    if (params.use_flow_trace) {
        fg = new FlowReader(num_flows, topology, params.cdf_or_flow_trace);
        fg->make_flows();
    }
    else if (params.interarrival_cdf != "none") {
        fg = new CustomCDFFlowGenerator(num_flows, topology, params.cdf_or_flow_trace, params.interarrival_cdf);
        fg->make_flows();
    }
    else if (params.permutation_tm != 0) {
        fg = new PermutationTM(num_flows, topology, params.cdf_or_flow_trace);
        fg->make_flows();
    }
    else if (params.bytes_mode) {
        fg = new PoissonFlowBytesGenerator(num_flows, topology, params.cdf_or_flow_trace);
        fg->make_flows();
    }
    else if (params.traffic_imbalance < 0.01) {
        fg = new PoissonFlowGenerator(num_flows, topology, params.cdf_or_flow_trace);
        fg->make_flows();
    }
    else {
        // TODO skew flow gen not yet implemented, need to move to FlowGenerator
        assert(false);
        //generate_flows_to_schedule_fd_with_skew(params.cdf_or_flow_trace, num_flows, topology);
    }

    if (params.deadline) {
        assign_flow_deadline(flows_to_schedule);
    }

    std::deque<Flow*> flows_sorted = flows_to_schedule;

    struct FlowComparator {
        bool operator() (Flow* a, Flow* b) {
            return a->start_time < b->start_time;
        }
    } fc;

    std::sort (flows_sorted.begin(), flows_sorted.end(), fc);

    for (uint32_t i = 0; i < flows_sorted.size(); i++) {
        Flow* f = flows_sorted[i];
        if (exp_type == GEN_ONLY) {
            std::cout << f->id << " " << f->size << " " << f->src->id << " " << f->dst->id << " " << 1e6*f->start_time << "\n";
        }
        else {
            flow_arrivals.push_back(new FlowArrivalEvent(f->start_time, f));
        }
    }

    if (exp_type == GEN_ONLY) {
        return;
    }

    //add_to_event_queue(new LoggingEvent((flows_sorted.front())->start_time));

    if (params.flow_type == FASTPASS_FLOW) {
        dynamic_cast<FastpassTopology*>(topology)->arbiter->start_arbiter();
    }

    // 
    // everything before this is setup; everything after is analysis
    //
    run_scenario();

    for (uint32_t i = 0; i < flows_sorted.size(); i++) {
        Flow *f = flows_to_schedule[i];
        validate_flow(f);
        if(!f->finished) {
            std::cout 
                << "unfinished flow " 
                << "size:" << f->size 
                << " id:" << f->id 
                << " next_seq:" << f->next_seq_no 
                << " recv:" << f->received_bytes  
                << " src:" << f->src->id 
                << " dst:" << f->dst->id 
                << "\n";
        }
    }

    //cleanup
    delete fg;
}
