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

#include "flow.h"
#include "turboflow.h"

#include "packet.h"
#include "node.h"
#include "event.h"
#include "topology.h"
#include "simpletopology.h"
#include "params.h"
#include "assert.h"
#include "queue.h"

#include "factory.h"
#include "random_variable.h"
#include "flow_generator.h"
#include "fountainflow.h"
#include "stats.h"
#include "capabilityflow.h"
#include "math.h"

extern Topology *topology;
extern double current_time;
extern std::priority_queue<Event *, std::vector<Event *>, EventComparator> event_queue;
extern std::deque<Flow *> flows_to_schedule;
extern std::deque<Event *> flow_arrivals;

extern uint32_t num_outstanding_packets;
extern uint32_t max_outstanding_packets;
extern DCExpParams params;
extern void add_to_event_queue(Event *);
extern void read_experiment_parameters(std::string conf_filename, uint32_t exp_type);
extern void read_flows_to_schedule(std::string filename, uint32_t num_lines, Topology *topo);
extern uint32_t duplicated_packets_received;

extern uint32_t num_outstanding_packets_at_50;
extern uint32_t num_outstanding_packets_at_100;
extern uint32_t arrival_packets_at_50;
extern uint32_t arrival_packets_at_100;

extern double start_time;
extern double get_current_time();

extern void printQueueStatistics(Topology *topo);
extern void run_scenario();

// alternate flow generations
/*
   int get_flow_size(Host* s, Host* d){

   int matrix[3][3] =
   {
   {3*1460,    3*1460, 700*1460},
   {3*1460,    0,      0},
   {700*1460,  0,      0}
   };

   assert(s->host_type >= 0);
   assert(d->host_type >= 0);
   assert(s->host_type < 3);
   assert(d->host_type < 3);

   return matrix[s->host_type][d->host_type];
   }

   int get_num_src_or_dst(Host* d){
   if(d->host_type == CPU)
   return 143;
   else if(d->host_type == MEM)
   return 144/3;
   else if(d->host_type == DISK)
   return 144/3;
   else
   assert(false);
   }


   void generate_flows_to_schedule_fd_ddc(std::string filename, uint32_t num_flows, Topology *topo) {



   for (int i = 0; i < topo->hosts.size(); i++){
   topo->hosts[i]->host_type = i%3;
   }



   for (uint32_t dst = 0; dst < topo->hosts.size(); dst++) {
   for (uint32_t src = 0; src < topo->hosts.size(); src++) {
   if (src != dst) {
   int flow_size = get_flow_size(topo->hosts[src], topo->hosts[dst]);

   if(flow_size > 0){
   int num_sd_pair;
   if(params.ddc_normalize == 0) {
   num_sd_pair = get_num_src_or_dst(topo->hosts[src]);
   }
   else if(params.ddc_normalize == 1) {
   num_sd_pair = get_num_src_or_dst(topo->hosts[dst]);
   }
   else if (params.ddc_normalize == 2) {
   if (topo->hosts[src]->host_type == CPU) {
   num_sd_pair = get_num_src_or_dst(topo->hosts[src]);
   }
   else if(topo->hosts[dst]->host_type == CPU) {
   num_sd_pair = get_num_src_or_dst(topo->hosts[dst]);
   }
   else {
   assert(false);
   }
   }
   else {
   assert(false);
   }

   double lambda_per_pair = params.bandwidth * params.load / (flow_size * 8.0 / 1460 * 1500) / num_sd_pair;
//std::cout << src << " " << dst << " " << flow_size << " " <<lambda_per_pair << "\n";
ExponentialRandomVariable *nv_intarr = new ExponentialRandomVariable(1.0 / lambda_per_pair);
double first_flow_time = 1.0 + nv_intarr->value();
EmpiricalRandomVariable *nv_bytes = new ConstantVariable(flow_size/1460);

    add_to_event_queue(
            new FlowCreationForInitializationEvent(first_flow_time, topo->hosts[src], topo->hosts[dst], nv_bytes, nv_intarr)
            );

    }
}
}
}

while (event_queue.size() > 0) {
    Event *ev = event_queue.top();
    event_queue.pop();
    current_time = ev->time;
    if (flows_to_schedule.size() < num_flows) {
        ev->process_event();
    }
    delete ev;
}
current_time = 0;
}


void generate_flows_to_schedule_fd_with_skew(std::string filename, uint32_t num_flows,
        Topology *topo) {

    EmpiricalRandomVariable *nv_bytes;
    if(params.smooth_cdf)
        nv_bytes = new EmpiricalRandomVariable(filename);
    else
        nv_bytes = new CDFRandomVariable(filename);

    params.mean_flow_size = nv_bytes->mean_flow_size;

    double lambda = params.bandwidth * params.load / (params.mean_flow_size * 8.0 / 1460 * 1500);



    GaussianRandomVariable popularity(10, params.traffic_imbalance);
    std::vector<int> sources;
    std::vector<int> destinations;

    int self_connection_count = 0;
    for(int i = 0; i < topo->hosts.size(); i++){
        int src_count = (int)(round(popularity.value()));
        int dst_count = (int)(round(popularity.value()));
        std::cout << "node:" << i << " #src:" << src_count << " #dst:" << dst_count << "\n";
        self_connection_count += src_count * dst_count;
        for(int j = 0; j < src_count; j++)
            sources.push_back(i);
        for(int j = 0; j < dst_count; j++)
            destinations.push_back(i);
    }

    double flows_per_host = (sources.size() * destinations.size() - self_connection_count) / (double)topo->hosts.size();
    double lambda_per_flow = lambda / flows_per_host;
    std::cout << "Lambda: " << lambda_per_flow << std::endl;


    ExponentialRandomVariable *nv_intarr;
    if(params.burst_at_beginning)
        nv_intarr = new ExponentialRandomVariable(0.000000001);
    else
        nv_intarr = new ExponentialRandomVariable(1.0 / lambda_per_flow);

    //* [expr ($link_rate*$load*1000000000)/($meanFlowSize*8.0/1460*1500)]
    for (uint32_t i = 0; i < sources.size(); i++) {
        for (uint32_t j = 0; j < destinations.size(); j++) {
            if (sources[i] != destinations[j]) {
                double first_flow_time = 1.0 + nv_intarr->value();
                add_to_event_queue(
                        new FlowCreationForInitializationEvent(first_flow_time,
                            topo->hosts[sources[i]], topo->hosts[destinations[j]],
                            nv_bytes, nv_intarr)
                        );
            }
        }
    }


    while (event_queue.size() > 0) {
        Event *ev = event_queue.top();
        event_queue.pop();
        current_time = ev->time;
        if (flows_to_schedule.size() < num_flows) {
            ev->process_event();
        }
        delete ev;
    }
    current_time = 0;
}
*/

void validate_flow(Flow* f){
    double slowdown = 1000000.0 * f->flow_completion_time / topology->get_oracle_fct(f);
    if(slowdown < 0.999999){
        std::cout << "Flow " << f->id << " has slowdown " << slowdown << "\n";
        //assert(false);
    }
    if(f->first_byte_send_time < 0 || f->first_byte_send_time < f->start_time - INFINITESIMAL_TIME)
        std::cout << "Flow " << f->id << " first_byte_send_time: " << f->first_byte_send_time << " start time:"
            << f->start_time << "\n";
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

    for (auto i = 0; i < topo->hosts.size(); i++) {
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
    int drop_ss = 0, drop_sl = 0, drop_ll = 0;
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

    if (params.cut_through == 1) {
        topology = new CutThroughTopology(params.num_hosts, params.num_agg_switches,
                params.num_core_switches, params.bandwidth, params.queue_type);
    } 
    else {
        if (params.big_switch) {
            topology = new BigSwitchTopology(params.num_hosts, params.bandwidth, params.queue_type);
        } else {
            topology = new PFabricTopology(params.num_hosts, params.num_agg_switches,
                    params.num_core_switches, params.bandwidth, params.queue_type);
        }
    }

    uint32_t num_flows = params.num_flows_to_run;

    FlowGenerator *fg;
    if (params.use_flow_trace) {
        fg = new FlowReader(num_flows, topology, params.cdf_or_flow_trace);
        fg->make_flows();
    }
    else {
        if (params.ddc) {
            // TODO ddc flow gen not yet implemented, need to move to FlowGenerator
            assert(false);
            //generate_flows_to_schedule_fd_ddc(params.cdf_or_flow_trace, num_flows, topology);
        }
        else if (params.traffic_imbalance < 0.01) {
            fg = new FlowGenerator(num_flows, topology, params.cdf_or_flow_trace);
            fg->make_flows();
        }
        else {
            // TODO skew flow gen not yet implemented, need to move to FlowGenerator
            assert(false);
            //generate_flows_to_schedule_fd_with_skew(params.cdf_or_flow_trace, num_flows, topology);
        }
    }

    if (params.deadline) {
        assign_flow_deadline(flows_to_schedule);
    }

    std::deque<Flow *> flows_sorted = flows_to_schedule;

    struct FlowComparator {
        bool operator() (Flow *a, Flow *b) {
            return a->start_time < b->start_time;
        }
    } fc;

    std::sort (flows_sorted.begin(), flows_sorted.end(), fc);

    for (uint32_t i = 0; i < flows_sorted.size(); i++) {
        Flow *f = flows_sorted[i];
        flow_arrivals.push_back(new FlowArrivalEvent(f->start_time, f));
    }

    add_to_event_queue(new LoggingEvent((flows_sorted.front())->start_time));

    std::cout 
        << "Running " << num_flows 
        << " Flows\nCDF_File " << params.cdf_or_flow_trace 
        << "\nBandwidth " << params.bandwidth/1e9 
        << "\nQueueSize " << params.queue_size 
        << "\nCutThrough " << params.cut_through 
        << "\nFlowType " << params.flow_type 
        << "\nQueueType " << params.queue_type 
        << "\nInit CWND " << params.initial_cwnd 
        << "\nMax CWND " << params.max_cwnd 
        << "\nRtx Timeout " << params.retx_timeout_value
        << "\nload_balancing (0: pkt)" << params.load_balancing 
        << std::endl;

    run_scenario();

    //write_flows_to_file(flows_sorted, "flow.tmp");

    Stats slowdown, inflation, fct, oracle_fct, first_send_time, slowdown_0_100, slowdown_100k_10m, slowdown_10m_inf;
    Stats data_pkt_sent, parity_pkt_sent, data_pkt_drop, parity_pkt_drop, deadline;
    std::map<unsigned, Stats*> slowdown_by_size, queuing_delay_by_size, capa_sent_by_size,
        fct_by_size, drop_rate_by_size, wait_time_by_size, first_hop_depart_by_size, last_hop_depart_by_size,
        capa_waste_by_size, log_slow_down_in_bytes;

    std::cout << std::setprecision(4) ;

    for (uint32_t i = 0; i < flows_sorted.size(); i++) {
        Flow *f = flows_to_schedule[i];
        validate_flow(f);
        if(!f->finished)
            std::cout 
                << "unfinished flow " 
                << "size:" << f->size 
                << " id:" << f->id 
                << " next_seq:" << f->next_seq_no 
                << " recv:" << f->received_bytes  
                << " src:" << f->src->id 
                << " dst:" << f->dst->id 
                << "\n";

        double slow = 1000000.0 * f->flow_completion_time / topology->get_oracle_fct(f);
        int meet_deadline = f->deadline > f->finish_time?1:0;
        if (slowdown_by_size.find(f->size_in_pkt) == slowdown_by_size.end()) {
            slowdown_by_size[f->size_in_pkt] = new Stats();
            queuing_delay_by_size[f->size_in_pkt] = new Stats();
            fct_by_size[f->size_in_pkt] = new Stats();
            drop_rate_by_size[f->size_in_pkt] = new Stats();
            wait_time_by_size[f->size_in_pkt] = new Stats();
            capa_sent_by_size[f->size_in_pkt] = new Stats();
            first_hop_depart_by_size[f->size_in_pkt] = new Stats();
            last_hop_depart_by_size[f->size_in_pkt] = new Stats();
            capa_waste_by_size[f->size_in_pkt] = new Stats();
        }
        int log_flow_size_in_bytes = (int)log10(f->size);
        if (log_slow_down_in_bytes.find(log_flow_size_in_bytes) == log_slow_down_in_bytes.end()) {
            log_slow_down_in_bytes[log_flow_size_in_bytes] = new Stats();
        }

        log_slow_down_in_bytes[log_flow_size_in_bytes]->input_data(slow);
        slowdown_by_size[f->size_in_pkt]->input_data(slow);
        queuing_delay_by_size[f->size_in_pkt]->input_data(f->get_avg_queuing_delay_in_us());
        fct_by_size[f->size_in_pkt]->input_data(f->flow_completion_time * 1000000);
        drop_rate_by_size[f->size_in_pkt]->input_data((double)(f->data_pkt_drop)/f->total_pkt_sent);
        wait_time_by_size[f->size_in_pkt]->input_data(f->first_byte_send_time - f->start_time);
        first_hop_depart_by_size[f->size_in_pkt]->input_data(f->first_hop_departure);
        last_hop_depart_by_size[f->size_in_pkt]->input_data(f->last_hop_departure);
        if (params.flow_type == CAPABILITY_FLOW) {
            capa_sent_by_size[f->size_in_pkt]->input_data(((CapabilityFlow*)f)->capability_count);
            capa_waste_by_size[f->size_in_pkt]->input_data(((CapabilityFlow*)f)->capability_waste_count);
        }

        if (params.deadline)
            deadline += meet_deadline;

        slowdown += slow;
        if (f->size < 100 * 1024)
            slowdown_0_100 += slow;
        else if (f->size < 10 * 1024 * 1024)
            slowdown_100k_10m += slow;
        else
            slowdown_10m_inf += slow;
        inflation += (double)f->total_pkt_sent / (f->size/f->mss);
        fct += (1000000.0 * f->flow_completion_time);
        oracle_fct += topology->get_oracle_fct(f);

        data_pkt_sent += std::min(f->size_in_pkt, (int)f->total_pkt_sent);
        parity_pkt_sent += std::max(0, (int)(f->total_pkt_sent - f->size_in_pkt));
        data_pkt_drop += f->data_pkt_drop;
        parity_pkt_drop += std::max(0, f->pkt_drop - f->data_pkt_drop);
    }

    double stability = ((double)num_outstanding_packets_at_100 - (double)num_outstanding_packets_at_50)/(arrival_packets_at_100 - arrival_packets_at_50);

    std::cout 
        << "AverageFCT " << fct.avg() 
        << " MeanSlowdown " << slowdown.avg() 
        << " MeanInflation " << inflation.avg() 
        << " NFCT " << fct.total() / oracle_fct.total() 
        << " Stability " << stability;

    if (params.deadline)
        std::cout << " DL:" << deadline.avg();

    std::cout << "\n";

    std::cout << "Slowdown Log Scale: ";
    for (auto it = log_slow_down_in_bytes.begin(); it != log_slow_down_in_bytes.end(); ++it) {
        std::cout << pow(10, it->first) << ":" << it->second->avg() << " ";
    }
    std::cout << "\n";

    int i = 0;
    for (auto it = slowdown_by_size.begin(); it != slowdown_by_size.end() && i < 6; ++it, ++i){
        unsigned key = it->first;
        std::cout 
            << key << ": Sl:" << it->second->avg() 
            <<  " FCT:" << fct_by_size[it->first]->avg() 
            << " QD:" << queuing_delay_by_size[it->first]->avg() 
            << "(" << queuing_delay_by_size[it->first]->sd() << ")" 
            << " Drp:" << drop_rate_by_size[it->first]->avg() 
            << " Wt:" << wait_time_by_size[it->first]->avg()*1000000;
        if (params.flow_type == CAPABILITY_FLOW)
            std::cout << " DC:" <<  (capa_sent_by_size[it->first]->total() - first_hop_depart_by_size[it->first]->total())/capa_sent_by_size[it->first]->total();
        std::cout << " DP:" << (first_hop_depart_by_size[it->first]->total() - last_hop_depart_by_size[it->first]->total())/first_hop_depart_by_size[it->first]->total();
        std::cout << " WST:" << capa_waste_by_size[it->first]->total()/capa_sent_by_size[it->first]->total();
        std::cout << "    ";
    }

    std::cout 
        << " [0,100k]avg: " << slowdown_0_100.avg() 
        << " [0,100k]99p: " << slowdown_0_100.get_percentile(0.99)
        << " [100k,10m]avg: " << slowdown_100k_10m.avg() 
        << " [100k,10m]99p: " << slowdown_100k_10m.get_percentile(0.99)
        << " [10m,inf]avg: " << slowdown_10m_inf.avg() 
        << " [10m,inf]99p: " << slowdown_10m_inf.get_percentile(0.99);
    << "\n";

    printQueueStatistics(topology);

    std::cout 
        << "Data Pkt Drop Rate: " << data_pkt_drop.total() / data_pkt_sent.total() 
        << " Parity Drop Rate:" << parity_pkt_drop.total()/parity_pkt_sent.total() 
        << "\n";

    //debug_flow_stats(flows_to_schedule);

    std::cout << params.param_str << "\n";
}

// TODO new experiment types (continuous flow gen) not yet implemented
/*
// Runs a initialized scenario 
void run_scenario_shuffle_traffic() {
// Flow Arrivals create new flow arrivals
// Add the first flow arrival

while (event_queue.size() > 0) {
Event *ev = event_queue.top();
event_queue.pop();
current_time = ev->time;
if (start_time < 0) {
start_time = current_time;
}
//event_queue.pop();
//std::cout << "main.cpp::run_scenario():" << get_current_time() << " Processing " << ev->type << " " << event_queue.size() << std::endl;
if (ev->cancelled) {
if(ev->unique_id == 7394)
std::cout << get_current_time() << " fixed..cpp:186 "  << " cancel " << ev->unique_id  << std::endl;
delete ev; //TODO: Smarter
continue;
}
ev->process_event();
delete ev;
}
}


EmpiricalRandomVariable *nv_bytes;



//same as run_pFabric_experiment except with this generate_flows.
void run_fixedDistribution_experiment_shuffle_traffic(int argc, char **argv, uint32_t exp_type) {
std::cout << "run_fixedDistribution_experiment_shuffle_traffic\n";
if (argc < 3) {
std::cout << "Usage: <exe> exp_type conf_file" << std::endl;
return;
}
std::string conf_filename(argv[2]);
read_experiment_parameters(conf_filename, exp_type);
params.num_hosts = 144;
params.num_agg_switches = 9;
params.num_core_switches = 4;


if (params.cut_through == 1) {
topology = new CutThroughTopology(params.num_hosts, params.num_agg_switches,
params.num_core_switches, params.bandwidth, params.queue_type);
} else {
topology = new PFabricTopology(params.num_hosts, params.num_agg_switches,
params.num_core_switches, params.bandwidth, params.queue_type);
}
PFabricTopology *topo = (PFabricTopology *) topology;


nv_bytes = new CDFRandomVariable(params.cdf_or_flow_trace);

//
//  uint32_t num_flows = params.num_flows_to_run;
//
//  //no reading flows to schedule in this mode.
//  generate_flows_to_schedule_fd(params.cdf_or_flow_trace, num_flows, topo);
//
//  std::deque<Flow *> flows_sorted = flows_to_schedule;
//  struct FlowComparator {
//    bool operator() (Flow *a, Flow *b) {
//      return a->start_time < b->start_time;
//    }
//  } fc;
//  std::sort (flows_sorted.begin(), flows_sorted.end(), fc);
//
//  for(uint32_t i = 0; i < flows_sorted.size(); i++) {
//    Flow *f = flows_sorted[i];
//    flow_arrivals.push_back(new FlowArrivalEvent(f->start_time, f));
//  }


int traffic_matrix[144];
for(int i = 0; i < 144; i++)
traffic_matrix[i] = i;

for(int i = 0; i < 144; i++){
    int j = std::rand()%(i+1);
    int s = traffic_matrix[i];
    traffic_matrix[i] = traffic_matrix[j];
    traffic_matrix[j] = s;
}

for(int i = 0; i < 144; i++){
    Host* src = topo->hosts[i];
    Host* dst = topo->hosts[traffic_matrix[i]];
    Flow* flow = Factory::get_flow(1, nv_bytes->value() * 1460, src, dst, params.flow_type);
    flows_to_schedule.push_back(flow);
    Event * event = new FlowArrivalEvent(flow->start_time, flow);
    add_to_event_queue(event);
}




add_to_event_queue(new LoggingEvent(1 + 0.01));

std::cout << "Running " << params.num_flows_to_run << " Flows\nCDF_File " <<
params.cdf_or_flow_trace << "\nBandwidth " << params.bandwidth/1e9 <<
"\nQueueSize " << params.queue_size <<
"\nCutThrough " << params.cut_through <<
"\nFlowType " << params.flow_type <<
"\nQueueType " << params.queue_type <<
"\nInit CWND " << params.initial_cwnd <<
"\nMax CWND " << params.max_cwnd <<
"\nRtx Timeout " << params.retx_timeout_value  <<
"\nload_balancing (0: pkt)" << params.load_balancing <<
std::endl;

run_scenario_shuffle_traffic();

std::deque<Flow *> flows_sorted = flows_to_schedule;
struct FlowComparator {
    bool operator() (Flow *a, Flow *b) {
        return a->start_time < b->start_time;
    }
} fc;
std::sort(flows_sorted.begin(), flows_sorted.end(), fc);

// print statistics
double sum = 0, sum_norm = 0, sum_inflation = 0;
for (uint32_t i = 0; i < flows_sorted.size(); i++) {
    Flow *f = flows_to_schedule[i];
    if(!f->finished)
        std::cout << "unfinished flow " << "size:" << f->size << " id:" << f->id << " next_seq:" << f->next_seq_no << " recv:" << f->received_bytes << "\n";
    sum += 1000000.0 * f->flow_completion_time;
    sum_norm += 1000000.0 * f->flow_completion_time / topology->get_oracle_fct(f);
    sum_inflation += (double)f->total_pkt_sent / (f->size/f->mss);
}
std::cout << "AverageFCT " << sum / flows_sorted.size() <<
" MeanSlowdown " << sum_norm / flows_sorted.size() <<
" MeanInflation " << sum_inflation / flows_sorted.size() <<
"\n";
printQueueStatistics(topo);
}
*/

