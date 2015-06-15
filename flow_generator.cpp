//
// flow_generator.cpp
// support arbitrary flow generation models.
//
// 6/15/2015 Akshay Narayan
//

#include "flow_generator.h"

FlowGenerator::FlowGenerator(uint32_t num_flows, Topology *topo, std::string filename) {
    this->num_flows = num_flows;
    this->topo = topo;
    this->filename = filename;
}

void FlowGenerator::write_flows_to_file(std::deque<Flow *> flows, std::string file){
    std::ofstream output(file);
    output.precision(20);
    for (uint i = 0; i < flows.size(); i++){
        output 
            << flows[i]->id << " " 
            << flows[i]->start_time << " " 
            << flows[i]->finish_time << " " 
            << flows[i]->size_in_pkt << " "
            << (flows[i]->finish_time - flows[i]->start_time) << " " 
            << 0 << " " 
            <<flows[i]->src->id << " " 
            << flows[i]->dst->id << "\n";
    }
    output.close();
}

//sample flow generation. Should be overridden.
void FlowGenerator::make_flows() {
    EmpiricalRandomVariable *nv_bytes = new EmpiricalRandomVariable(filename);
    ExponentialRandomVariable *nv_intarr = new ExponentialRandomVariable(0.0000001);
    add_to_event_queue(new FlowCreationForInitializationEvent(1.0, topo->hosts[0], topo->hosts[1], nv_bytes, nv_intarr));

    while (event_queue.size() > 0) {
        Event *ev = event_queue.top();
        event_queue.pop();
        current_time = ev->time;
        if (flows_to_schedule.size() < 10) {
            ev->process_event();
        }
        delete ev;
    }
    current_time = 0;
}

PoissonFlowGenerator::PoissonFlowGenerator(uint32_t num_flows, Topology *topo, std::string filename) : FlowGenerator(num_flows, topo, filename) {};
    
void PoissonFlowGenerator::make_flows() {
    EmpiricalRandomVariable *nv_bytes;
    if (params.smooth_cdf)
        nv_bytes = new EmpiricalRandomVariable(filename);
    else
        nv_bytes = new CDFRandomVariable(filename);

    params.mean_flow_size = nv_bytes->mean_flow_size;

    double lambda = params.bandwidth * params.load / (params.mean_flow_size * 8.0 / 1460 * 1500);
    double lambda_per_host = lambda / (topo->hosts.size() - 1);
    std::cout << "Lambda: " << lambda_per_host << std::endl;


    ExponentialRandomVariable *nv_intarr;
    if (params.burst_at_beginning)
        nv_intarr = new ExponentialRandomVariable(0.0000001);
    else
        nv_intarr = new ExponentialRandomVariable(1.0 / lambda_per_host);

    //* [expr ($link_rate*$load*1000000000)/($meanFlowSize*8.0/1460*1500)]
    for (uint32_t i = 0; i < topo->hosts.size(); i++) {
        for (uint32_t j = 0; j < topo->hosts.size(); j++) {
            if (i != j) {
                double first_flow_time = 1.0 + nv_intarr->value();
                add_to_event_queue(
                    new FlowCreationForInitializationEvent(
                        first_flow_time,
                        topo->hosts[i], 
                        topo->hosts[j],
                        nv_bytes, 
                        nv_intarr
                    )
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

FlowReader::FlowReader(uint32_t num_flows, Topology *topo, std::string filename) : FlowGenerator(num_flows, topo, filename) {};

void FlowReader::make_flows() {
    std::ifstream input(filename);
    while(!input.eof()) {
        double start_time, temp;
        uint32_t size, s, d;
        uint32_t id;
        
        // <id> <start_time> blah blah <size in packets> blah blah <src> <dst>

        if ((input >> id).eof())
            break;
        input >> start_time;
        input >> temp;
        input >> temp; size = (uint32_t) (params.mss * temp);
        input >> temp; input >> temp;
        input >> s >> d;

        std::cout << "Flow " << id << " " << start_time << " " << size << " " << s << " " << d << "\n";
        flows_to_schedule.push_back(
            Factory::get_flow(id, start_time, size, topo->hosts[s], topo->hosts[d], params.flow_type)
        );
    }
    params.num_flows_to_run = flows_to_schedule.size();
    input.close();
}

