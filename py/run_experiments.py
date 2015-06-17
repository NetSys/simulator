#!/usr/bin/python

import sys
import subprocess
import threading
import multiprocessing

def run_exp(conf_str, experiments, exp_name, bin_location, exp_type, semaphore):
    semaphore.acquire()
    conf_file_str = conf_str.format(*experiments[exp_name])
    conf_file_name = 'conf_' + exp_name + '.txt'
    with open(conf_file_name, 'w') as conf_file:
        conf_file.write(conf_file_str)
    result_file_name = 'results_' + exp_name + '.txt'
    with open(result_file_name, 'w') as res_file:
        print bin_location, exp_type, conf_file_name
        subprocess.call([bin_location, str(exp_type), conf_file_name], stdout = res_file)
    semaphore.release()

def run_experiments(bin_location, exp_type, conf_str, exps):
    threads = []
    semaphore = threading.Semaphore(multiprocessing.cpu_count())
    for exp_name in exps:
        threads.append(threading.Thread(target=run_exp, args=(conf_str, exps, exp_name, bin_location, exp_type, semaphore)))
    [t.start() for t in threads]
    [t.join() for t in threads]
    print 'finished', len(threads), 'experiments'

