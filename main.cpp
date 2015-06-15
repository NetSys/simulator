#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <deque>
#include <stdint.h>
#include <time.h>

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
#include "fastpasshost.h"

using namespace std;


Topology *topology;
double current_time = 0;
std::priority_queue<Event *, std::vector<Event *>, EventComparator> event_queue;
std::deque<Flow *> flows_to_schedule;
std::deque<Event *> flow_arrivals;

uint32_t num_outstanding_packets;
uint32_t max_outstanding_packets;
uint32_t num_outstanding_packets_at_50;
uint32_t num_outstanding_packets_at_100;
uint32_t arrival_packets_at_50;
uint32_t arrival_packets_at_100;
uint32_t arrival_packets_count = 0;

uint32_t total_finished_flows = 0;

extern DCExpParams params;
extern double get_current_time();
extern void add_to_event_queue(Event *);
extern int get_event_queue_size();
uint32_t duplicated_packets_received = 0;
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


/* Runs a initialized scenario */
void run_scenario() {
  if(params.flow_type == FASTPASS_FLOW)
      ((PFabricTopology*)topology)->arbiter->start_arbiter();
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
    delete ev;

    if(last_evt_type == ev->type)
      same_evt_count++;
    else
      same_evt_count = 0;

    last_evt_type = ev->type;

    if(same_evt_count > 1000){
      std::cout << "Ended event dead loop. Type:" << last_evt_type << "\n";
      break;
    }
  }
}


extern void run_pFabric_experiment(int argc, char **argv, uint32_t exp_type);
extern void run_continuous_experiment(int argc, char **argv);
extern void run_fixedDistribution_experiment(int argc, char **argv, uint32_t exp_type);
extern void run_fixedDistribution_experiment_shuffle_traffic(int argc, char **argv, uint32_t exp_type);
extern void run_single_link_experiment(int argc, char ** argv);
extern void run_single_sender_receiver_exp(int argc, char ** argv);
extern void run_nto1_experiment(int argc, char ** argv);

int main (int argc, char ** argv) {
  time_t start_time;
  time(&start_time);

  //srand(time(NULL));
  srand(0);
  std::cout.precision(15);

  uint32_t exp_type = atoi(argv[1]);
  switch (exp_type) {
    case DC_EXP_WITH_TRACE:
    case DC_EXP_WITHOUT_TRACE:
      run_pFabric_experiment(argc, argv, exp_type);
      break;
    case DC_EXP_CONTINUOUS_FLOWMODEL:
      run_continuous_experiment(argc, argv);
      break;
    case DC_EXP_UNIFORM_FLOWS:
      run_fixedDistribution_experiment(argc, argv, exp_type);
      break;
    case DC_EXP_UNIFORM_FLOWS_SHUFFLE_TRAFFIC:
      run_fixedDistribution_experiment_shuffle_traffic(argc, argv, exp_type);
      break;
    case SINGLE_LINK_EXP_IONSTYLE:
      run_single_link_experiment(argc, argv);
      break;
    case SINGLE_LINK_EXP_SYLVIASTYLE:
      run_single_sender_receiver_exp(argc, argv);
      break;
    case N_TO_1_EXP:
      run_nto1_experiment(argc, argv);
      break;
  }

  time_t end_time;
  time(&end_time);
  double duration = difftime(end_time, start_time);
  cout << currentDateTime() << " Simulator ended. Execution time: " << duration << " seconds\n";
}
