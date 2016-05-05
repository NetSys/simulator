#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <deque>
#include <stdint.h>
#include <time.h>
#include "assert.h"

#include "flow.h"
#include "packet.h"
#include "node.h"
#include "event.h"
#include "topology.h"
#include "queue.h"
#include "random_variable.h"

#include "../ext/factory.h"
//#include "../ext/fastpasshost.h"

#include "../run/params.h"

using namespace std;

Topology* topology;
double current_time = 0;
std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
std::deque<Flow*> flows_to_schedule;
std::deque<Event*> flow_arrivals;

uint32_t num_outstanding_packets = 0;
uint32_t max_outstanding_packets = 0;
uint32_t num_outstanding_packets_at_50 = 0;
uint32_t num_outstanding_packets_at_100 = 0;
uint32_t arrival_packets_at_50 = 0;
uint32_t arrival_packets_at_100 = 0;
uint32_t arrival_packets_count = 0;
uint32_t total_finished_flows = 0;
uint32_t duplicated_packets_received = 0;

uint32_t injected_packets = 0;
uint32_t duplicated_packets = 0;
uint32_t dead_packets = 0;
uint32_t completed_packets = 0;
uint32_t backlog3 = 0;
uint32_t backlog4 = 0;
uint32_t total_completed_packets = 0;
uint32_t sent_packets = 0;

extern DCExpParams params;
double start_time = -1;

const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void add_to_event_queue(Event* ev) {
    event_queue.push(ev);
}

int get_event_queue_size() {
    return event_queue.size();
}

double get_current_time() {
    return current_time; // in us
}

/* Runs a initialized scenario */
void run_scenario() {
    // Flow Arrivals create new flow arrivals
    // Add the first flow arrival
    if (flow_arrivals.size() > 0) {
        add_to_event_queue(flow_arrivals.front());
        flow_arrivals.pop_front();
    }
    int last_evt_type = -1;
    int same_evt_count = 0;
    while (event_queue.size() > 0) {
        Event *ev = event_queue.top();
        event_queue.pop();
        current_time = ev->time;
        if (start_time < 0) {
            start_time = current_time;
        }
        if (ev->cancelled) {
            delete ev; //TODO: Smarter
            continue;
        }
        ev->process_event();

        if(last_evt_type == ev->type && last_evt_type != 9)
            same_evt_count++;
        else
            same_evt_count = 0;

        last_evt_type = ev->type;
        
        if(same_evt_count > 100000){
            std::cout << "Ended event dead loop. Type:" << last_evt_type << "\n";
            break;
        }

        delete ev;
    }
}

extern void run_experiment(int argc, char** argv, uint32_t exp_type);

int main (int argc, char ** argv) {
    time_t start_time;
    time(&start_time);

    //srand(time(NULL));
    srand(0);
    std::cout.precision(15);

    uint32_t exp_type = atoi(argv[1]);
    switch (exp_type) {
        case GEN_ONLY:
        case DEFAULT_EXP:
            run_experiment(argc, argv, exp_type);
            break;
        default:
            assert(false);
    }

    time_t end_time;
    time(&end_time);
    double duration = difftime(end_time, start_time);
    cout << currentDateTime() << " Simulator ended. Execution time: " << duration << " seconds\n";
}

