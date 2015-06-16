#ifndef OTHER_EVENTS_H
#define OTHER_EVENTS_H

#include "../core/event.h"

#define HOST_PROCESSING 10
#define CAPABILITY_PROCESSING 11
#define MAGIC_HOST_SCHEDULE 12
#define SENDER_NOTIFY 13
#define ARBITER_PROCESSING 14
#define FASTPASS_FLOW_PROCESSING 15
#define FASTPASS_TIMEOUT 16

class CapabilityHost;
class CapabilityFlow;
class SchedulingHost;
class MagicHost;
class MagicFlow;
class FastpassArbiter;
class FastpassFlow;

class HostProcessingEvent : public Event {
    public:
        HostProcessingEvent(double time, SchedulingHost *host);
        ~HostProcessingEvent();
        void process_event();
        SchedulingHost *host;
};

class CapabilityProcessingEvent : public Event {
    public:
        CapabilityProcessingEvent(double time, CapabilityHost *host, bool is_timeout);
        ~CapabilityProcessingEvent();
        void process_event();
        CapabilityHost *host;
        bool is_timeout_evt;
};

class MagicHostScheduleEvent : public Event {
    public:
        MagicHostScheduleEvent(double time, MagicHost *host);
        ~MagicHostScheduleEvent();
        void process_event();
        MagicHost *host;
};

class SenderNotifyEvent : public Event {
    public:
        SenderNotifyEvent(double time, CapabilityHost *host);
        ~SenderNotifyEvent();
        void process_event();
        CapabilityHost *host;
};

class ArbiterProcessingEvent : public Event {
    public:
        ArbiterProcessingEvent(double time, FastpassArbiter *host);
        ~ArbiterProcessingEvent();
        void process_event();
        FastpassArbiter *arbiter;
};

class FastpassFlowProcessingEvent : public Event {
    public:
        FastpassFlowProcessingEvent(double time, FastpassFlow *flow);
        ~FastpassFlowProcessingEvent();
        void process_event();
        FastpassFlow* flow;
};

class FastpassTimeoutEvent : public Event {
    public:
        FastpassTimeoutEvent(double time, FastpassFlow *flow);
        ~FastpassTimeoutEvent();
        void process_event();
        FastpassFlow* flow;
};

#endif
