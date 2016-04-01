#ifndef IDEAL_H
#define IDEAL_H

#include "../coresim/flow.h"
#include "schedulinghost.h"

#include "../run/params.h"

#include <array>

class IdealFlow;

class IdealArbiter {
    public:
        std::vector<IdealFlow*>* active_flows;
        std::array<bool, 144>* srcs;
        std::array<bool, 144>* dsts;

        IdealArbiter();
        void flow_arrival(IdealFlow* f);
        void flow_finished(IdealFlow* f);
        void compute_schedule();
};

class IdealHost : public SchedulingHost {
    public:
        IdealHost(uint32_t id, double rate, uint32_t queue_type);
        IdealArbiter* dispatch;
        IdealFlow* active_flow;

        virtual void start(Flow* f);
        virtual void send();
};

class IdealAck : public PlainAck {
    public:
        IdealAck(Flow* flow, uint32_t num_received, Host* src, Host* dst);
        uint32_t received;
};

class IdealFlow : public Flow {
    public:
        IdealFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);

        uint32_t sent;     // Sender side
        uint32_t acked;    

        uint32_t received; // Receiver side

        virtual void start_flow();
        virtual void handle_timeout();
        virtual void send_pending_data();
        virtual void receive(Packet* p);
        virtual uint32_t get_priority(uint32_t seq);
};

#endif
