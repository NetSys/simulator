#ifndef FACTORY_H
#define FACTORY_H


#include "flow.h"
#include "node.h"
#include "queue.h"

/* Queue types */
#define DROPTAIL_QUEUE 1
#define PFABRIC_QUEUE 2
#define TURBO_QUEUE 3
#define PROB_DROP_QUEUE 4

/* Flow types */
#define NORMAL_FLOW 1
#define PFABRIC_FLOW 2
#define FULLBLAST_PACED_FLOW 3
#define TURBO_FLOW 4
#define TURBO_FLOW_STOP_ON_TIMEOUT 5
#define TURBO_FLOW_PERPACKET_TIMEOUT 6
#define TURBO_FLOW_PERPACKET_TIMEOUT_WITHPROBING 7
#define TURBO_FLOW_PERPACKET_TIMEOUT_WITHRAREPROBING 8
#define TURBO_FLOW_LONGFLOWS_LOW 9

#define PFABRIC_FLOW_NO_SLOWSTART 15

#define PACED_FLOW 20
#define JITTERED_PACED_FLOW 21

#define FOUNTAIN_FLOW 100
#define FOUNTAIN_FLOW_SCHEDULING_HOST 101
#define RTSCTS_FLOW 102
#define FOUNTAIN_FLOW_PIPELINE_SCHEDULING_HOST 111
#define CAPABILITY_FLOW 112
#define MAGIC_FLOW 113
#define FASTPASS_FLOW 114

/* Host types */
#define NORMAL_HOST 1
#define SCHEDULING_HOST 2
#define RTSCTS_HOST 3
#define PIPELINE_SCHEDULING_HOST 11
#define CAPABILITY_HOST 12
#define MAGIC_HOST 13
#define FASTPASS_HOST 14
#define FASTPASS_ARBITER 10

class Factory {
public:
  static int flow_counter;
  static Flow *get_flow(uint32_t id, double start_time, uint32_t size,
                        Host *src, Host *dst, uint32_t flow_type,
                        double paced_rate = 0.0);
  static Flow *get_flow(double start_time, uint32_t size,
                        Host *src, Host *dst, uint32_t flow_type,
                        double paced_rate = 0.0);
  static Queue *get_queue(uint32_t id, double rate,
                          uint32_t queue_size, uint32_t type,
                          double drop_prob, int location);
  static Host* get_host(uint32_t id, double rate, uint32_t queue_type, uint32_t host_type);
};

#endif
