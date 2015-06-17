#!/usr/bin/python

import sys
import csv
import itertools
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt


def getAllResults(*csvFileNames):
    results = []
    for csvFileName in csvFileNames[0]:
        with open(csvFileName) as f:
            csvread = csv.reader(f)
            for line in csvread:
                name, dist, load, qsize, slowdown, nfct, shortFct, _99th_sfct, longFct = line
                load = float(load)
                qsize = int(qsize[5:])
                slowdown = float(slowdown)
                nfct = float(nfct)

                results.append((name, dist, load, qsize, slowdown, nfct, shortFct, _99th_sfct, longFct))

    return results

def plot_xload(name, results):
    phost_results = [x for x in results if x[0] == 'pHost']
    pfabric_results = [x for x in results if x[0] == 'pFabric']
    pH_q36 = [x for x in phost_results if x[3] == 36864]
    pF_q36 = [x for x in pfabric_results if x[3] == 36864]
    pH_q36.sort(key=lambda x:x[2]) #sort by load since load is on xaxis
    pF_q36.sort(key=lambda x:x[2])
    loads = [0.5, 0.6, 0.7, 0.8, 0.9]
    plt.cla()
    plt.plot(loads, [x[8] for x in pH_q36], 'bo-', label = 'pHost')
    plt.plot(loads, [x[8] for x in pF_q36], 'ro-', label = 'pFabric')
    plt.legend(loc = 'upper left')
    plt.xlabel('Load')
    plt.ylabel('Average FCT')
    plt.title('[10MB, +inf)')
    plt.savefig(name+'.png')

def plot_distributions(results):
    aditya = [x for x in results if x[1] == 'aditya']
    dctcp = [x for x in results if x[1] == 'dctcp']
    datamining = [x for x in results if x[1] == 'datamining']
    #plot('99_fct_aditya', aditya)
    plot_xload('long_fct_dctcp', dctcp)
    plot_xload('long_fct_datamining', datamining)

def plot_xfracs(name, results):
    pH_q36 = [x for x in results if x[0] == 'pHost' and x[3] == 36864]
    pF_q36 = [x for x in results if x[0] == 'pFabric' and x[3] == 36864]
    pH_q36.sort(key=lambda x:x[1])
    pF_q36.sort(key=lambda x:x[1])

    fracs = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]

    plt.cla()
    plt.plot(fracs, [x[5] for x in pH_q36], 'bo-', label='pHost')
    plt.plot(fracs, [x[5] for x in pF_q36], 'ro-', label='pFabric')
    plt.legend(loc = 'upper left')
    plt.xlabel('Percentage of Short Flows')
    plt.ylabel('Average FCT')
    plt.savefig(name+'.png')

def plot_bimodals(results):
    load6 = [x for x in results if x[2] == 0.6]
    load8 = [x for x in results if x[2] == 0.8]

    plot_xfracs('bimodal_0.6', load6)
    plot_xfracs('bimodal_0.8', load8)

if (len(sys.argv) >= 2):
    csvFiles = sys.argv[1:]
    results = getAllResults(csvFiles)
    plot_bimodals(results)

else:
    print 'Usage: python plot_results.py <csv out files>'

