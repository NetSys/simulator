#ifndef CAPABILITY_HOST_H
#define CAPABILITY_HOST_H

#include <set>
#include <queue>

#include "../coresim/node.h"
#include "../coresim/packet.h"
#include "../coresim/event.h"

#include "schedulinghost.h"
#include "custompriorityqueue.h"

class CapabilityFlow;
class CapabilityProcessingEvent;
class SenderNotifyEvent;

class CapabilityFlowComparator {
    public:
        bool operator() (CapabilityFlow* a, CapabilityFlow* b);
};

class CapabilityFlowComparatorAtReceiver {
    public:
        bool operator() (CapabilityFlow* a, CapabilityFlow* b);
};

class CapabilityHost : public SchedulingHost {
    public:
        CapabilityHost(uint32_t id, double rate, uint32_t queue_type);
        void schedule_host_proc_evt();
        void start_capability_flow(CapabilityFlow* f);
        void send();
        //std::priority_queue<CapabilityFlow*, std::vector<CapabilityFlow*>, CapabilityFlowComparator> active_sending_flows;
        CustomPriorityQueue<CapabilityFlow*, std::vector<CapabilityFlow*>, CapabilityFlowComparator> active_sending_flows;

        void send_capability();
        void schedule_capa_proc_evt(double time, bool is_timeout);
        void schedule_sender_notify_evt();
        bool check_better_schedule(CapabilityFlow* f);
        bool is_sender_idle();
        void notify_flow_status();
        //std::priority_queue<CapabilityFlow*, std::vector<CapabilityFlow*>, CapabilityFlowComparatorAtReceiver> active_receiving_flows;
        CustomPriorityQueue<CapabilityFlow*, std::vector<CapabilityFlow*>, CapabilityFlowComparatorAtReceiver> active_receiving_flows;
        CapabilityProcessingEvent *capa_proc_evt;
        SenderNotifyEvent* sender_notify_evt;
        int hold_on;
        int total_capa_schd_evt_count;
        int could_better_schd_count;
};

#define CAPABILITY_PROCESSING 11
class CapabilityProcessingEvent : public Event {
    public:
        CapabilityProcessingEvent(double time, CapabilityHost *host, bool is_timeout);
        ~CapabilityProcessingEvent();
        void process_event();
        CapabilityHost *host;
        bool is_timeout_evt;
};

#define SENDER_NOTIFY 13
class SenderNotifyEvent : public Event {
    public:
        SenderNotifyEvent(double time, CapabilityHost *host);
        ~SenderNotifyEvent();
        void process_event();
        CapabilityHost *host;
};

#endif
