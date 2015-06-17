#ifndef SCHEDULING_HOST_H
#define SCHEDULING_HOST_H

#include <set>
#include <queue>

#include "../coresim/node.h"
#include "../coresim/packet.h"

class Flow;
class HostProcessingEvent;

class HostFlowComparator {
    public:
        bool operator() (Flow* a, Flow* b);
};

class SchedulingHost : public Host {
    public:
        SchedulingHost(uint32_t id, double rate, uint32_t queue_type);
        virtual void start(Flow* f);
        virtual void send();
        std::priority_queue<Flow*, std::vector<Flow*>, HostFlowComparator> sending_flows;
        HostProcessingEvent* host_proc_event;
};

#endif
