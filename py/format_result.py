#!/usr/bin/python

import sys
import operator
import subprocess
import numpy as np
import csv

def write_result(fname, outfile):
    blurb = blurbGetter(fname)
    flows = flowsGetter(fname) # only slowdowns

    name = fname.split('_')
    slug = name[1]
    dist = name[2]
    load = name[3]
    qsize = name[4].split('.tx')[0]

    smallFlows = [x for x in flows if int(x[0]) < 10240]
    largeFlows = [x for x in flows if int(x[0]) > 50000]
    sizes, fcts, oracles, slowdowns = zip(*flows)

    afct = np.mean(fcts)
    mean_oracle = np.mean(oracles)
    slowdown = np.mean(slowdowns)

    nfct = afct/mean_oracle

    smallFlowsAvg = np.mean([x[1] for x in smallFlows])
    if (len(largeFlows) > 0):
        largeFlowsAvg = np.mean([x[1] for x in largeFlows])
    else:
        largeFlowsAvg = 0
    smallFlows.sort(key=lambda x:float(x[1]))
    _99th_smalls = float(smallFlows[int(len(smallFlows)*0.99)][1])

    with open(outfile, 'a') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow([slug, dist, load, qsize, slowdown, nfct, smallFlowsAvg, _99th_smalls, largeFlowsAvg])

def blurbGetter(fname):
    fout = subprocess.check_output(['tail', '-7', fname])
    return fout.split('\n')

def flowsGetter(fname):
    fout = subprocess.check_output(['egrep', '^[0-9]+ [0-9]+', fname])
    '''<id> <size> <src> <dst> <start> <end> <fct> <oracle> <slowdown> <drops> blah blah'''
    def op(x):
        sp = x.split()
        return tuple(map(float, (sp[1], sp[6], sp[7], sp[8])))
    return [op(x) for x in fout.split('\n') if len(x.split()) >= 9] # slowdown is [8], fct is [6]

if (len(sys.argv) > 1):
    outfile = sys.argv[1]
    for result in sys.argv[2:]:
        write_result(result, outfile)
