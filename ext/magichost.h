#ifndef MAGIC_HOST_H
#define MAGIC_HOST_H

#include <set>
#include <queue>
#include <map>

#include "../coresim/node.h"
#include "../coresim/packet.h"
#include "../coresim/event.h"

#include "schedulinghost.h"

class MagicFlow;
class MagicHostScheduleEvent;

class MagicHostFlowComparator {
    public:
        bool operator() (MagicFlow* a, MagicFlow* b);
};

class MagicFlowTimeoutComparator{
    public:
        bool operator() (MagicFlow* a, MagicFlow* b);
};

class MagicHost : public SchedulingHost {
    public:
        MagicHost(uint32_t id, double rate, uint32_t queue_type);
        void start(Flow* f);
        void schedule();
        void reschedule();
        void try_send();
        void send();
        Flow* flow_sending;
        MagicFlow* flow_receiving;
        //Flow* flow_receiving;
        double recv_busy_until;
        bool is_host_proc_event_a_timeout;
        std::priority_queue<MagicFlow*, std::vector<MagicFlow*>, MagicHostFlowComparator> active_sending_flows;
        std::priority_queue<MagicFlow*, std::vector<MagicFlow*>, MagicFlowTimeoutComparator> sending_redundency;
        std::map<uint32_t, MagicFlow*> receiver_pending_flows;
};

#define MAGIC_HOST_SCHEDULE 12
class MagicHostScheduleEvent : public Event {
    public:
        MagicHostScheduleEvent(double time, MagicHost *host);
        ~MagicHostScheduleEvent();
        void process_event();
        MagicHost *host;
};

#endif
