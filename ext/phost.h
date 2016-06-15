#ifndef P_HOST_H
#define P_HOST_H

#include <set>
#include <queue>

#include "../coresim/node.h"
#include "../coresim/packet.h"
#include "../coresim/event.h"

#include "schedulinghost.h"
#include "custompriorityqueue.h"

#include "phostflow.h"

class PHost;

class PHostToken {
    public:
        PHostFlow *flow;
        uint32_t seqno;
        double timeout;
        PHostToken(PHostFlow* f, uint32_t seq, double to) {
            flow = f;
            seqno = seq;
            timeout = to;
        };
};

class PHostFlowComparator {
    public:
        bool operator() (PHostToken* a, PHostToken* b);
};

class PHostReceiverFlowComparator {
    public:
        bool operator() (PHostFlow* a, PHostFlow* b);
};

#define PHOST_TOKEN_PROCESSING_EVENT 20
class PHostTokenProcessingEvent : public Event {
    public:
        PHostTokenProcessingEvent(double time, PHost *host);
        ~PHostTokenProcessingEvent();
        void process_event();
        PHost *host;
};

class PHost : public SchedulingHost {
    public:
        PHost(uint32_t id, double rate, uint32_t queue_type);

        // sender side
        CustomPriorityQueue<PHostToken*, std::vector<PHostToken*>, PHostFlowComparator> received_capabilities;
        virtual void send(); // called by HostProcessingEvent

        // receiver side
        void start_receiving(Flow* f);
        CustomPriorityQueue<PHostFlow*, std::vector<PHostFlow*>, PHostReceiverFlowComparator> active_receiving_flows;
        PHostTokenProcessingEvent* capa_proc_evt;
        void send_capability();
};

#endif
