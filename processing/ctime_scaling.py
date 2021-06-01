import sys
import os
import itertools
import matplotlib.pyplot as plt
import statistics
import math
import numpy as np
import sys

apps = ["bt", "cg", "ep", "ft", "lu", "mg", "sp", "ua", "dc"]
nums = [44, 308, 660, 880, 1056] 

slurm_overall_meds = {}
slurm_overall_maxs = {}
penelope_overall_meds = {}
penelope_overall_maxs = {}

def process_app_pair(app):
    system = "slurm"
    slurm_xs = []
    slurm_meds = []
    slurm_q1s, slurm_q3s = [], []
    slurm_maxs = []
    for num in nums:
        name = system + "_scaling_" + str(num) + "_1" 
        filename = name + "/slurm_ctime_100"
        try:
            with open(filename) as f:
                lines = [float(line.rstrip()) for line in f.readlines()]
                if len(lines) == 3:
                    slurm_q1s.append(lines[0])
                    slurm_meds.append(lines[1])
                    slurm_q3s.append(lines[2])
                    slurm_maxs.append(180)
                elif len(lines) < 3:
                    slurm_q1s.append(180)
                    slurm_meds.append(180)
                    slurm_q3s.append(180)
                    slurm_maxs.append(180)
                else:
                    slurm_q1s.append(lines[0])
                    slurm_meds.append(lines[1])
                    slurm_q3s.append(lines[2])
                    slurm_maxs.append(lines[3])
                slurm_xs.append(num)
        except:
            slurm_q1s.append(180)
            slurm_meds.append(180)
            slurm_q3s.append(180)
            slurm_maxs.append(180)
            slurm_xs.append(num)
            continue
    
    pen_xs = []
    pen_meds = []
    pen_q1s, pen_q3s = [], []
    pen_maxs = []
    
    fails = 0
    system = "penelope"
    for num in nums:
        name = system + "_scaling_" + str(num) + "_1000" 
        os.chdir(name)
        print(name)
        avgs = []
        for fname in os.listdir("ctime"):
            filename = "ctime/" + fname
            try:
                with open(filename) as f:
                    line = f.readline().rstrip()
                    avgs.append(float(line))
            except Exception as e:
                fails += 1
                avgs.append(180)
                continue
    
        a = np.array(avgs)
        n = len(avgs)
        if n != 0: 
            m1 = min(avgs)
            m2 = max(avgs)
            avg = np.average(a)
            median = np.quantile(a, 0.5)
            q1 = np.quantile(a, 0.25)
            q3 = np.quantile(a, 0.75)
    
            pen_xs.append(num)
            pen_meds.append(median)
            pen_q1s.append(q1)
            pen_q3s.append(q3)
            pen_maxs.append(m2)
            print(f"F: {num}::: min={m1}, max={m2}, avg={avg}, median={median}")
        else:
            print("ERROR")
    
        print(f"failures:{fails}")
        os.chdir("..")
    
    penelope_overall_maxs[app] = pen_maxs
    penelope_overall_meds[app] = pen_meds
    slurm_overall_maxs[app] = slurm_maxs
    slurm_overall_meds[app] = slurm_meds

    
def process_apps():
    for pair in itertools.combinations(apps, 2):
        app1, app2 = pair
        app_dir = app1 + "_" + app2
        print(app_dir)
        os.chdir(app_dir)
        process_app_pair(app_dir)
        os.chdir("..")

def graph_median():
    penelope='#a22a2b'
    slurm='#357e99'
    navy='#20334e'
    orange='#e58727'
    background='#d5d4c2'

    keys = [app1 + "_" + app2 for app1, app2 in itertools.combinations(apps, 2)]

    slurm_medians = np.array([slurm_overall_meds[app] for app in keys])
    penelope_medians = np.array([penelope_overall_meds[app] for app in keys])
    pq1s = np.quantile(penelope_medians, 0.25, axis=0)
    pq3s = np.quantile(penelope_medians, 0.75, axis=0) 
    pmed = np.median(penelope_medians, axis=0)
    sq1s = np.quantile(slurm_medians, 0.25, axis=0)   
    sq3s = np.quantile(slurm_medians, 0.75, axis=0)  
    smed = np.median(slurm_medians, axis=0)

    plt.plot(nums, smed, label="Slurm", color=slurm)
    plt.fill_between(nums, sq1s, sq3s, alpha=0.5, edgecolor=slurm, facecolor=slurm)

    plt.plot(nums, pmed, label="Penelope", color=penelope)
    plt.fill_between(nums, pq1s, pq3s, alpha=0.5, edgecolor=penelope, facecolor=penelope)

    plt.xlabel("Number of Nodes")
    plt.ylabel("Power Redistribution Time (seconds)")
    plt.title("Median Redistribution Time vs. Scale")
    plt.legend()
    filename = "median_distrib.png"
    plt.savefig(filename)

def graph_max():
    penelope='#a22a2b'
    slurm='#357e99'
    navy='#20334e'
    orange='#e58727'
    background='#d5d4c2'

    keys = [app1 + "_" + app2 for app1, app2 in itertools.combinations(apps, 2)]

    slurm_maxs = np.array([slurm_overall_maxs[app] for app in keys])
    penelope_maxs = np.array([penelope_overall_maxs[app] for app in keys])
    pq1s = np.quantile(penelope_maxs, 0.25, axis=0)
    pq3s = np.quantile(penelope_maxs, 0.75, axis=0) 
    pmed = np.median(penelope_maxs, axis=0)
    sq1s = np.quantile(slurm_maxs, 0.25, axis=0)   
    sq3s = np.quantile(slurm_maxs, 0.75, axis=0)  
    smed = np.median(slurm_maxs, axis=0)

    plt.clf()
    plt.plot(nums, smed, label="Slurm", color=slurm)
    plt.fill_between(nums, sq1s, sq3s, alpha=0.5, edgecolor=slurm, facecolor=slurm)

    plt.plot(nums, pmed, label="Penelope", color=penelope)
    plt.fill_between(nums, pq1s, pq3s, alpha=0.5, edgecolor=penelope, facecolor=penelope)

    plt.xlabel("Number of Nodes")
    plt.ylabel("Power Redistribution Time (seconds)")
    plt.title("Total Redistribution Time vs. Scale")
    plt.legend()
    filename = "max_distrib.png"
    plt.savefig(filename)


if __name__ == '__main__':
    data_path = sys.argv[1]
    home = os.path.expanduser("~")
    path = home + "/" + data_path
    os.chdir(path)

    process_apps()
    graph_median()
    graph_max()

