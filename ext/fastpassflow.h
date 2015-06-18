#ifndef FASTPASS_FLOW_H
#define FASTPASS_FLOW_H

#include <unordered_map>
#include <set>

#include "../coresim/flow.h"

class FastpassEpochSchedule;

class FastpassFlow : public Flow {
public:
    FastpassFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);
    void start_flow();
    void update_remaining_size();
    void send_ack_pkt(uint32_t);
    void send_schedule_pkt(FastpassEpochSchedule* schd);
    void send_data_pkt();
    void receive(Packet *p);
    void schedule_send_pkt(double time);
    int next_pkt_to_send();
    void fastpass_timeout();

    int sender_remaining_num_pkts;
    std::set<int> sender_acked;
    std::set<int> receiver_received;
    int sender_acked_count;
    int sender_acked_until;
    int sender_last_pkt_sent;
    bool sender_finished;
    int arbiter_remaining_num_pkts;
    bool arbiter_received_rts;
    bool arbiter_finished;
};

#define FASTPASS_FLOW_PROCESSING 15
class FastpassFlowProcessingEvent : public Event {
    public:
        FastpassFlowProcessingEvent(double time, FastpassFlow *flow);
        ~FastpassFlowProcessingEvent();
        void process_event();
        FastpassFlow* flow;
};

#define FASTPASS_TIMEOUT 16
class FastpassTimeoutEvent : public Event {
    public:
        FastpassTimeoutEvent(double time, FastpassFlow *flow);
        ~FastpassTimeoutEvent();
        void process_event();
        FastpassFlow* flow;
};

#endif
