#!/usr/bin/python

import os.path
import sys
sys.path.append("../")
from run_experiments import run_experiments

conf_str = '''init_cwnd: {}
max_cwnd: {}
retx_timeout: {}
queue_size: {}
propagation_delay: 0.0000002
bandwidth: 10000000000.0
queue_type: {}
flow_type: {}
num_flow: 100000
flow_trace: {}
cut_through: 1
mean_flow_size: {}
load_balancing: 0
preemptive_queue: 0
big_switch: 0
host_type: {}
traffic_imbalance: {}
load: {}
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
capability_window_timeout: 15
ddc: 0
ddc_cpu_ratio: 0.33
ddc_mem_ratio: 0.33
ddc_disk_ratio: 0.34
ddc_normalize: 2
deadline: 0
schedule_by_deadline: 0
avg_deadline: 0.0001
capability_third_level: {}
capability_fourth_level: 0
magic_inflate: 1

'''

cdf_file = '''3 1 {0}
700 1 1
'''


def get_mean_flow_size(cdf_file):
    sys.path.append("../")
    from calculate_mean_flowsize import get_mean_flowsize
    return int(get_mean_flowsize(cdf_file))

def make_experiment(qsize, qtype, ftype, trace, htype, skew, load, withprio):
    initCwnd, maxCwnd, rto, _ = pfabric_qsize_params(qsize)
    return (initCwnd, maxCwnd, rto, qsize, qtype, ftype, trace, get_mean_flow_size(trace), htype, skew, load, withprio)

def pfabric_qsize_params(qsize):
    rto = lambda qsize: 2.27982e-6 + 1.16455e-9*qsize
    init_cwnd = lambda qsize:18084/7621 + (2*qsize)/7621
    allowed_sizes = (4096, 6200, 9216, 18432, 36864, 73728)
    assert(qsize in allowed_sizes)
    initCwnd = init_cwnd(qsize)
    return (initCwnd, initCwnd + 3, rto(qsize), qsize)

def make_cdfs():
    fracs = map(lambda x:float(x/10.0), xrange(1,10))
    cdfs = []
    for frac in fracs:
        cdf = cdf_file.format(frac)
        name = '../CDF_{0}short.txt'.format(frac)
        if (not os.path.exists(name)):
            with open(name, 'w') as f:
                f.write(cdf)
        cdfs.append((frac,name))
    return cdfs

def cdf_experiments():
    cdfs = make_cdfs()
    skew = 0
    loads = [0.5, 0.6, 0.7, 0.8, 0.9]
    qsizes = [73728, 36864, 18432, 9216, 4096]
    qtype = 2
    ftype = 112
    htype = 12

    experiments = {}

    for qs in qsizes:
        for l in loads:
            for frac, cdfFile in cdfs:
                expName = '{0}_{1}short_{2}_queue{3}'.format('pHost', frac, l, qs)
                experiments[expName] = make_experiment(qs, qtype, ftype, cdfFile, htype, skew, l)

    qtype = 2
    ftype = 2
    htype = 1

    for qs in qsizes:
        for l in loads:
            for frac, cdfFile in cdfs:
                expName = '{0}_{1}short_{2}_queue{3}'.format('pFabric', frac, l, qs)
                experiments[expName] = make_experiment(qs, qtype, ftype, cdfFile, htype, skew, l, 1)

    return experiments

def distr_experiments_phost(name):
    qtype = 2
    ftype = 112
    htype = 12
    skew = 0

    #qsizes = [73728, 36864, 18432, 9216, 4096]
    qsizes = [6200, 36864]
    traces = ['../CDF_dctcp.txt', '../CDF_aditya.txt']
    loads = [0.5, 0.6, 0.7, 0.8, 0.9]

    experiments = {}
    for qs in qsizes:
        for l in loads:
            for trace in traces:
                expName = '{0}NoPrio_{1}_{2}_queue{3}'.format(name, trace[:-4].split('_')[-1], l, qs)
                experiments[expName] = make_experiment(qs, qtype, ftype, trace, htype, skew, l, 0)

    return experiments

def distr_experiments_pfabric(name):
    qtype = 2
    ftype = 2
    htype = 1
    skew = 0

    #qsizes = [73728, 36864, 18432, 9216, 4096]
    qsizes = [6200, 36864]
    traces = ['../CDF_dctcp.txt', '../CDF_aditya.txt']
    loads = [0.5, 0.6, 0.7, 0.8, 0.9]

    experiments = {}
    for qs in qsizes:
        for l in loads:
            for trace in traces:
                expName = '{0}_{1}_{2}_queue{3}'.format(name, trace[:-4].split('_')[-1], l, qs)
                experiments[expName] = make_experiment(qs, qtype, ftype, trace, htype, skew, l, 1)

    return experiments

def distr_experiments_magic():
    name = 'inflMagic'
    qtype = 2
    ftype = 113
    htype = 13
    skew = 0

    #qsizes = [73728, 36864, 18432, 9216, 4096]
    qsizes = [6200, 36864]
    traces = ['../CDF_dctcp.txt', '../CDF_aditya.txt']
    loads = [0.5, 0.6, 0.7, 0.8, 0.9]

    experiments = {}
    for qs in qsizes:
        for l in loads:
            for trace in traces:
                expName = '{0}_{1}_{2}_queue{3}'.format(name, trace[:-4].split('_')[-1], l, qs)
                experiments[expName] = make_experiment(qs, qtype, ftype, trace, htype, skew, l, 1)

    return experiments

if __name__ == '__main__':
    exps = {}
    exps.update(distr_experiments_phost('pHost'))
    #exps.update(distr_experiments_pfabric('pFabric'))
    #exps.update(distr_experiments_magic())


    run_experiments('../simulator', 1, conf_str, exps)

