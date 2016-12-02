#ifndef CAPABILITY_FLOW_H
#define CAPABILITY_FLOW_H

#include <map>
#include <set>

#include "fountainflow.h"
#include "custompriorityqueue.h"

struct Capability //for extendability
{
    double timeout;
    int seq_num;
    bool has_idle_sibling_sender;
    int data_seq_num;
};

class CapabilityComparator{
public:
    bool operator() (Capability* a, Capability* b);
};


class CapabilityFlow : public FountainFlow {
public:
    CapabilityFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);
    virtual void start_flow();
    virtual void send_pending_data();
    void receive_rts(Packet *p);
    virtual void receive(Packet *p);
    void send_pending_data_low_prio();
    Packet* send(uint32_t seq, int capa_seq, int data_seq, int priority);
    void send_capability_pkt();
    void send_notify_pkt(int);
    void send_rts_pkt();
    bool has_capability();
    int use_capability();
    Capability* top_capability();
    double top_capability_timeout();
    int remaining_pkts();
    void assign_init_capability();
    void set_capability_count();
    int capability_gap();
    void relax_capability_gap();
    int init_capa_size();
    bool has_sibling_idle_source();
    int get_next_capa_seq_num();

    std::priority_queue<Capability*, std::vector<Capability*>, CapabilityComparator> capabilities;
    std::set<int> packets_received;
    int last_capa_data_seq_num_sent;
    int received_until;
    bool finished_at_receiver;
    int capability_count;
    int capability_packet_sent_count;
    int capability_waste_count;
    double redundancy_ctrl_timeout;
    int capability_goal;
    int remaining_pkts_at_sender;
    int largest_cap_seq_received;
    double latest_cap_sent_time;
    bool rts_received;
    double latest_data_pkt_send_time;
    int notified_num_flow_at_sender;
};


#endif

