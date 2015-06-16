#ifndef PARAMS_H
#define PARAMS_H

#include <string>
#include <fstream>

class DCExpParams {
    public:
        std::string param_str;

        int initial_cwnd;
        int max_cwnd;
        double retx_timeout_value;
        int mss;
        int hdr_size;
        int queue_size;
        int queue_type;
        int flow_type;
        int load_balancing; //0 per pkt, 1 per flow

        double propagation_delay;
        double bandwidth;

        int num_flows_to_run;
        double end_time;
        std::string cdf_or_flow_trace;
        int cut_through;
        int mean_flow_size;


        int num_hosts;
        int num_agg_switches;
        int num_core_switches;
        int preemptive_queue;
        int big_switch;
        int host_type;
        double traffic_imbalance;
        double load;

        double reauth_limit;

        double magic_trans_slack;
        int magic_delay_scheduling;
        int magic_inflate;

        int use_flow_trace;
        int smooth_cdf;
        int burst_at_beginning;
        double capability_timeout;
        double capability_resend_timeout;
        int capability_initial;
        int capability_window;
        int capability_prio_thresh;
        double capability_window_timeout;
        int capability_third_level;
        int capability_fourth_level;

        int ddc;
        double ddc_cpu_ratio;
        double ddc_mem_ratio;
        double ddc_disk_ratio;
        int ddc_normalize; //0: sender send, 1: receiver side, 2: both

        int deadline;
        int schedule_by_deadline;
        double avg_deadline;

        double get_full_pkt_tran_delay(int size_in_byte = 1500)
        {
            return size_in_byte * 8 / this->bandwidth;
        }

};


#define CAPABILITY_MEASURE_WASTE false
#define CAPABILITY_NOTIFY_BLOCKING false
#define CAPABILITY_HOLD false

#define FASTPASS_EPOCH_TIME 0.000010
#define FASTPASS_EPOCH_PKTS 8

void read_experiment_parameters(std::string conf_filename, uint32_t exp_type); 

/* General main function */
#define DEFAULT_EXP 1

#define INFINITESIMAL_TIME 0.000000000001

#endif
