#!/usr/bin/python

import subprocess
import threading
import multiprocessing

conf_str_pfabric = '''init_cwnd: 12
max_cwnd: 15
retx_timeout: 45e-06
queue_size: 36864
propagation_delay: 0.0000002
bandwidth: 40000000000.0
queue_type: 2
flow_type: 2
num_flow: {0}
flow_trace: ./{1}
cut_through: 1
mean_flow_size: 0
load_balancing: 0
preemptive_queue: 0
big_switch: 0
host_type: 1
traffic_imbalance: 0
load: 0.6
reauth_limit: 3
magic_trans_slack: 1.1
magic_delay_scheduling: 1
use_flow_trace: 0
smooth_cdf: 1
burst_at_beginning: 0
capability_timeout: 1.5
capability_resend_timeout: 9
capability_initial: 8
capability_window: 8
capability_window_timeout: 25
ddc: 0
ddc_cpu_ratio: 0.33
ddc_mem_ratio: 0.33
ddc_disk_ratio: 0.34
ddc_normalize: 2
ddc_type: 0
deadline: 0
schedule_by_deadline: 0
avg_deadline: 0.0001
capability_third_level: 1
capability_fourth_level: 0
magic_inflate: 1
interarrival_cdf: none
num_host_types: 13
permutation_tm: 1

'''

conf_str_phost = '''init_cwnd: 2
max_cwnd: 6
retx_timeout: 9.50003e-06
queue_size: 36864
propagation_delay: 0.0000002
bandwidth: 40000000000.0
queue_type: 2
flow_type: 112
num_flow: {0}
flow_trace: ./{1}
cut_through: 1
mean_flow_size: 0
load_balancing: 0
preemptive_queue: 0
big_switch: 0
host_type: 12
traffic_imbalance: 0
load: 0.6
reauth_limit: 3
magic_trans_slack: 1.1
magic_delay_scheduling: 1
use_flow_trace: 0
smooth_cdf: 1
burst_at_beginning: 0
capability_timeout: 1.5
capability_resend_timeout: 9
capability_initial: 8
capability_window: 8
capability_window_timeout: 25
ddc: 0
ddc_cpu_ratio: 0.33
ddc_mem_ratio: 0.33
ddc_disk_ratio: 0.34
ddc_normalize: 2
ddc_type: 0
deadline: 0
schedule_by_deadline: 0
avg_deadline: 0.0001
capability_third_level: 1
capability_fourth_level: 0
magic_inflate: 1
interarrival_cdf: none
num_host_types: 13
permutation_tm: 1

'''

conf_str_fastpass = '''init_cwnd: 6
max_cwnd: 12
retx_timeout: 45e-06
queue_size: 36864
propagation_delay: 0.0000002
bandwidth: 40000000000.0
queue_type: 2
flow_type: 114
num_flow: {0}
flow_trace: ./{1}
cut_through: 1
mean_flow_size: 0
load_balancing: 0
preemptive_queue: 0
big_switch: 0
host_type: 14
traffic_imbalance: 0
load: 0.6
reauth_limit: 3
magic_trans_slack: 1.1
magic_delay_scheduling: 1
use_flow_trace: 0
smooth_cdf: 1
burst_at_beginning: 0
capability_timeout: 1.5
capability_resend_timeout: 9
capability_initial: 8
capability_window: 8
capability_window_timeout: 25
ddc: 0
ddc_cpu_ratio: 0.33
ddc_mem_ratio: 0.33
ddc_disk_ratio: 0.34
ddc_normalize: 2
ddc_type: 0
deadline: 0
schedule_by_deadline: 0
avg_deadline: 0.0001
capability_third_level: 1
capability_fourth_level: 0
magic_inflate: 1
interarrival_cdf: none
num_host_types: 13
permutation_tm: 1

'''


template = '../simulator 1 conf_{0}_{1}.txt > result_{0}_{1}.txt'
cdf_temp = './CDF_{}.txt'

runs = ['pfabric', 'phost', 'fastpass']
workloads = ['aditya', 'dctcp', 'datamining']

def getNumLines(trace):
    out = subprocess.check_output('wc -l {}'.format(trace), shell=True)
    return int(out.split()[0])


def run_exp(rw, semaphore):
    semaphore.acquire()
    print template.format(*rw)
    subprocess.call(template.format(*rw), shell=True)
    semaphore.release()

threads = []
semaphore = threading.Semaphore(multiprocessing.cpu_count())

for r in runs:
    for w in workloads:
        cdf = cdf_temp.format(w)
        numLines = 1000000

        #  generate conf file
        if r == 'pfabric':
            conf_str = conf_str_pfabric.format(numLines, cdf)
        elif r == 'phost':
            conf_str = conf_str_phost.format(numLines, cdf)
        elif r == 'fastpass':
            conf_str = conf_str_fastpass.format(numLines, cdf)
        else:
            assert False, r

        confFile = "conf_{0}_{1}.txt".format(r, w)
        with open(confFile, 'w') as f:
            print confFile
            f.write(conf_str)

        threads.append(threading.Thread(target=run_exp, args=((r, w), semaphore)))

print '\n'
[t.start() for t in threads]
[t.join() for t in threads]
print 'finished', len(threads), 'experiments'
