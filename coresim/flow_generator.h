//
// flow_generator.h
// support arbitrary flow generation models.
//
// 6/15/2015 Akshay Narayan
//

#ifndef FLOW_GEN_H
#define FLOW_GEN_H

#include <iostream>
#include <algortihm>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include "math.h"
#include "assert.h"

#include "params.h"
#include "flow.h"
#include "event.h"
#include "node.h"
#include "topology.h"
#include "queue.h"
#include "random_variable.h"

#include "../ext/factory.h"

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

class FlowReader : public FlowGenerator {
public:
    FlowReader(uint32_t num_flows, Topology *topo, std::string filename);
    virtual void make_flows();
};

#endif
