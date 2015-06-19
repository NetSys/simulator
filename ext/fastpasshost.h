#ifndef FASTPASS_HOST_H
#define FASTPASS_HOST_H

#include <map>
#include <set>

#include "../coresim/node.h"
#include "../coresim/packet.h"
#include "../coresim/event.h"

#include "../run/params.h"

class ArbiterProcessingEvent;
class FastpassFlow;

class FastpassFlowComparator {
    public:
        bool operator() (FastpassFlow* a, FastpassFlow* b);
};

class FastpassEpochSchedule {
    public:
        FastpassEpochSchedule(double s);
        FastpassFlow* get_sender();
        double start_time;
        std::map<int, FastpassFlow*> schedule;
};

class FastpassHost : public Host {
    public:
        FastpassHost(uint32_t id, double rate, uint32_t queue_type);
        void receive_schedule_pkt(FastpassSchedulePkt* pkt);
};

class FastpassArbiter : public Host {
    public:
        FastpassArbiter(uint32_t id, double rate, uint32_t queue_type);
        void start_arbiter();
        void schedule_proc_evt(double time);
        std::map<int, FastpassFlow*> schedule_timeslot();
        void schedule_epoch();
        void receive_rts(FastpassRTS* rts);

        ArbiterProcessingEvent* arbiter_proc_evt;
        std::priority_queue<FastpassFlow*, std::vector<FastpassFlow*>, FastpassFlowComparator> sending_flows;
};

#define ARBITER_PROCESSING 14
class ArbiterProcessingEvent : public Event {
    public:
        ArbiterProcessingEvent(double time, FastpassArbiter *host);
        ~ArbiterProcessingEvent();
        void process_event();
        FastpassArbiter* arbiter;
};

#endif
