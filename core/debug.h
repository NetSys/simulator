#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

bool debug_flow(uint32_t fid);
bool debug_queue(uint32_t fid);
bool debug_host(uint32_t fid);
bool debug();
bool print_flow_result();
#endif
