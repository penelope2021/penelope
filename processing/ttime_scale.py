# import sys
import math
import os
import itertools
import statistics
import matplotlib.pyplot as plt
import numpy as np
import sys

presentation_mode = int(sys.argv[1])
apps = ["bt", "cg", "ep", "ft", "lu", "mg", "sp", "ua", "dc"]
systems = [ "penelope", "slurm"]
freqs = ["_1000", "_1"]
nums = [44, 308, 660, 880, 1056] 


penelope_devs = {}
penelope_vals = {}
slurm_devs = {}
slurm_vals = {}

def process_file(filename):
    try:
        with open(filename) as f:
            lines = [float(x.rstrip()) for x in f.readlines()]
            lines.sort()
            n = len(lines)
            ind1 = math.floor((n-1)/2)
            ind2 = math.ceil((n-1)/2)
            median = (lines[ind1] + lines[ind2]) / 2
            return median
    except Exception as e:
        raise e

def process_app_pair(app):
    xs = [ [], [] ]
    vals = [ [], [] ]
    devs = [ [], [] ]
    for system in systems:
        ind = systems.index(system)
        for num in nums:
            name = system + "_scaling_" + str(num) + freqs[ind]
            print(name)
            clients = [x for x in range(num)]
            avgs = []
            for c in clients:
                try:
                    filename = name + "/ttime/" + system + "_ttime_" + str(c)
                    val = process_file(filename)
                    avgs.append(val)
                except Exception as e:
                    print('error')
                    continue
            x = sum(avgs)
            n = len(avgs)
            if n != 0: 
                avg = x/n
                std = statistics.stdev(avgs)
                vals[ind].append(avg)
                xs[ind].append(num)
                devs[ind].append(std)
        
    
    vals[1] = [x * 1000000 for x in vals[1]]
    devs[1] = [x * 1000000 for x in devs[1]]

    penelope_vals[app] = vals[0]
    penelope_devs[app] = devs[0]
    slurm_vals[app] = vals[1]
    slurm_devs[app] = devs[1]

def process_apps():
    for pair in itertools.combinations(apps, 2):
        app1, app2 = pair
        app_dir = app1 + "_" + app2
        print(app_dir)
        os.chdir(app_dir)
        process_app_pair(app_dir)
        os.chdir("..")

def graph():
    penelope='#a22a2b'
    slurm='#357e99'
    navy='#20334e'
    orange='#e58727'
    background='#d5d4c2'
    
    keys = [app1 + "_" + app2 for app1, app2 in itertools.combinations(apps, 2)]

    slurm_stds = np.array([slurm_devs[app] for app in keys])
    penelope_stds = np.array([penelope_devs[app] for app in keys])

    slurm_ttimes = np.array([slurm_vals[app] for app in keys])
    penelope_ttimes = np.array([penelope_vals[app] for app in keys])
    
    smed = np.median(slurm_ttimes, axis=0)
    sq1s = np.quantile(slurm_ttimes, 0.25, axis=0)
    sq3s = np.quantile(slurm_ttimes, 0.75, axis=0)
    pmed = np.median(penelope_ttimes, axis=0)
    pq1s = np.quantile(penelope_ttimes, 0.25, axis=0)
    pq3s = np.quantile(penelope_ttimes, 0.75, axis=0)

    plt.plot(nums, smed, label="Slurm", color=slurm)
    plt.fill_between(nums, sq1s, sq3s, alpha=0.5, edgecolor=slurm, facecolor=slurm)
    
    plt.plot(nums, pmed, label="Penelope", color=penelope)
    plt.fill_between(nums, pq1s, pq3s, alpha=0.5, edgecolor=penelope, facecolor=penelope)

    plt.xlabel("Number of Nodes")
    plt.ylabel("Average Turnaround Time (Microseconds)")
    plt.title("Mean Turnaround Time vs. Scale")

    plt.legend()
    plt.savefig("ttime_errorbars.png")


if __name__ == '__main__':
    data_path = sys.argv[1]
    home = os.path.expanduser("~")
    path = home + "/" + data_path
    os.chdir(path)

    process_apps()
    graph()
