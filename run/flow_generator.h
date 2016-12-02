//
// flow_generator.h
// support arbitrary flow generation models.
//
// 6/15/2015 Akshay Narayan
//

#ifndef FLOW_GEN_H
#define FLOW_GEN_H

#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <set>
#include "math.h"
#include "assert.h"

#include "../coresim/flow.h"
#include "../coresim/event.h"
#include "../coresim/node.h"
#include "../coresim/topology.h"
#include "../coresim/queue.h"
#include "../coresim/random_variable.h"

#include "../ext/factory.h"

#include "params.h"

extern Topology *topology;
extern double current_time;
extern std::priority_queue<Event *, std::vector<Event *>, EventComparator> event_queue;
extern std::deque<Flow *> flows_to_schedule;
extern std::deque<Event *> flow_arrivals;

extern DCExpParams params;
extern void add_to_event_queue(Event *);

extern double start_time;
extern double get_current_time();

// subclass FlowGenerator to implement your favorite flow generation scheme

class FlowGenerator {
public:
    uint32_t num_flows;
    Topology *topo;
    std::string filename;

    FlowGenerator(uint32_t num_flows, Topology *topo, std::string filename);
    virtual void write_flows_to_file(std::deque<Flow*> flows, std::string file);
    virtual void make_flows();
};

class PoissonFlowGenerator : public FlowGenerator {
public:
    PoissonFlowGenerator(uint32_t num_flows, Topology *topo, std::string filename);
    virtual void make_flows();
};

class PoissonFlowBytesGenerator : public FlowGenerator {
public:
    PoissonFlowBytesGenerator(uint32_t num_flows, Topology *topo, std::string filename);
    virtual void make_flows();
};

class FlowReader : public FlowGenerator {
public:
    FlowReader(uint32_t num_flows, Topology *topo, std::string filename);
    virtual void make_flows();
};

class CustomCDFFlowGenerator : public FlowGenerator {
public:
    CustomCDFFlowGenerator(uint32_t num_flows, Topology *topo, std::string filename, std::string interarrivals_cdf_filename);
    virtual void make_flows();

    std::string interarrivals_cdf_filename;
private:
    std::vector<EmpiricalRandomVariable*>* makeCDFArray(std::string fn_template, std::string filename);
};

class PermutationTM : public FlowGenerator {
public:
    PermutationTM(uint32_t num_flows, Topology *topo, std::string filename);
    virtual void make_flows();

};

#endif
