#ifndef MAGIC_FLOW_H
#define MAGIC_FLOW_H

#include <assert.h>
#include "fountainflow.h"

class MagicFlow : public FountainFlowWithSchedulingHost {
    public:
        MagicFlow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);
        virtual void start_flow();
        virtual void receive(Packet *p);
        virtual void send_pending_data();
        double estimated_finish_time();
        unsigned remaining_pkt();
        double get_propa_time();
        Packet* send(uint32_t seq);
        double schedule_time;
        double ack_timeout;
        int send_count;
        int virtual_rts_send_count;
        int remaining_pkt_this_round;
        double last_pkt_sent_at;
        double total_waiting_time;

        bool added_infl_time;
};

#endif
