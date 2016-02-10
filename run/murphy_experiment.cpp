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

#include "../ext/paced_flow.h"
#include "../ext/murphy_topology.h"

extern DCExpParams params;
extern void read_experiment_parameters(std::string conf_filename, uint32_t exp_type);

extern double current_time;
extern double start_time;
extern double get_current_time();

extern void add_to_event_queue(Event*);
extern std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
extern void run_scenario();

extern Topology *topology;

void run_murphy_experiment(int argc, char **argv, uint32_t exp_type) {
    if (argc < 3) {
        std::cout << "Usage: <exe> exp_type conf_file" << std::endl;
        return;
    }

    std::string conf_filename(argv[2]);
    read_experiment_parameters(conf_filename, exp_type);

    topology = new MurphyTopology();

    Flow* f = new PacedFlow(0, get_current_time(), 0, ((MurphyTopology*) topology)->h1, ((MurphyTopology*) topology)->h2);
    add_to_event_queue(new FlowArrivalEvent(get_current_time(), f));

    std::cout << "running scenario\n";
    run_scenario();
}
