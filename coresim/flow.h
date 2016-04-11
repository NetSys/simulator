#ifndef FLOW_H
#define FLOW_H

#include <unordered_map>
#include "node.h"

class Packet;
class Ack;
class Probe;
class RetxTimeoutEvent;
class FlowProcessingEvent;

class Flow {
    public:
        Flow(uint32_t id, double start_time, uint32_t size, Host *s, Host *d);

        ~Flow(); // Destructor

        virtual void start_flow();
        virtual void send_pending_data();
        virtual Packet *send(uint32_t seq);
        virtual void send_ack(uint32_t seq, std::vector<uint32_t> sack_list);
        virtual void receive_ack(uint32_t ack, std::vector<uint32_t> sack_list);
        void receive_data_pkt(Packet* p);
        virtual void receive(Packet *p);
        
        // Only sets the timeout if needed; i.e., flow hasn't finished
        virtual void set_timeout(double time);
        virtual void handle_timeout();
        virtual void cancel_retx_event();

        virtual uint32_t get_priority(uint32_t seq);
        virtual void increase_cwnd();
        virtual double get_avg_queuing_delay_in_us();

        uint32_t id;
        double start_time;
        double finish_time;
        uint32_t size;
        Host *src;
        Host *dst;
        uint32_t cwnd_mss;
        uint32_t max_cwnd;
        double retx_timeout;
        uint32_t mss;
        uint32_t hdr_size;

        // Sender variables
        uint32_t next_seq_no;
        uint32_t last_unacked_seq;
        RetxTimeoutEvent *retx_event;
        FlowProcessingEvent *flow_proc_event;

        //  std::unordered_map<uint32_t, Packet *> packets;

        // Receiver variables
        std::unordered_map<uint32_t, bool> received;
        uint32_t received_bytes;
        uint32_t recv_till;
        uint32_t max_seq_no_recv;

        uint32_t total_pkt_sent;
        int size_in_pkt;
        int pkt_drop;
        int data_pkt_drop;
        int ack_pkt_drop;
        int first_hop_departure;
        int last_hop_departure;
        uint32_t received_count;
        // Sack
        uint32_t scoreboard_sack_bytes;
        // finished variables
        bool finished;
        double flow_completion_time;
        double total_queuing_time;
        double first_byte_send_time;
        double first_byte_receive_time;

        uint32_t flow_priority;
        double deadline;
};

#endif
