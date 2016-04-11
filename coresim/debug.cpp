#include "debug.h"
#include <set>

extern double get_current_time();

bool debug_mode = true;
bool print_flow = true;
double debug_start_time = 0;

bool debug_all_flows = false;
std::set<uint32_t> flows_to_debug_set = {};
bool debug_all_queues = false;
std::set<uint32_t> queues_to_debug_set = {};
bool debug_all_hosts = false;
std::set<uint32_t> hosts_to_debug_set = {};

bool debug_flow(uint32_t fid){
    return false;
    //return debug_mode && get_current_time() >= debug_start_time?(debug_all_flows||flows_to_debug_set.count(fid)):false;
}


bool debug_queue(uint32_t qid){
    return debug_mode && get_current_time() >= debug_start_time?(debug_all_queues||queues_to_debug_set.count(qid)):false;
}

bool debug_host(uint32_t qid){
    return debug_mode &&  get_current_time() >= debug_start_time?(debug_all_hosts||hosts_to_debug_set.count(qid)):false;
}

bool debug(){
    return debug_mode &&  get_current_time() >= debug_start_time;
}

bool print_flow_result(){
    return print_flow;
}
