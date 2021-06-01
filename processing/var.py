import math
import os
import itertools
import statistics
import matplotlib.pyplot as plt
import numpy as np
import sys

apps = ["bt", "cg", "ep", "ft", "lu", "mg", "sp", "ua", "dc"]
systems = ["penelope", "slurm"]
freqs = ["_1000", "_1"]
nums = [44, 308, 660, 880, 1056] 

penelope_vars = {}
slurm_vars = {}

def process_file(filename):
    try:
        with open(filename) as f:
            lines = [float(x.split(",")[1].rstrip()) for x in f.readlines()]
            vals = []
            for i in range(1, len(lines)):
                if lines[i] < lines[i-1]:
                    diff = lines[i-1] - lines[i]
                    vals.append(diff)
            return vals
    except Exception as e:
        raise e


def process_app_pair(app):
    variances = {"penelope":[], "slurm":[]}
    means = {"penelope":[], "slurm":[]}
    medians = {"penelope":[], "slurm":[]}
    stds = {"penelope":[], "slurm":[]}
    for system in systems:
        ind = systems.index(system)
        for num in nums:
            name = system + "_scaling_" + str(num) + freqs[ind]
            os.chdir(name)
            print(name)
            transactions = []

            if system == "penelope":
                for fname in os.listdir("pool"):
                    filename = "pool/" + fname
                    try:
                        vals = process_file(filename)
                        transactions.extend(vals)
                    except Exception as e:
                        raise e
            else:
                filename = "slurm_pool_100"
                transactions = process_file(filename)

            os.chdir("..")
            np_trans = np.array(transactions)
            var = np.var(np_trans)
            means[system].append(np.mean(np_trans))
            medians[system].append(np.median(np_trans))
            stds[system].append(np.std(np_trans))
            variances[system].append(var)
    
    penelope_vars[app] = variances["penelope"]
    slurm_vars[app] = variances["slurm"]
    # penelope_vars[app] = medians["penelope"]
    # slurm_vars[app] = medians["slurm"]


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

    variances1 = np.array([penelope_vars[app] for app in keys])
    variances2 = np.array([slurm_vars[app] for app in keys])

    pmed = np.median(variances1, axis=0)
    pq1s = np.quantile(variances1, 0.25, axis=0)
    pq3s = np.quantile(variances1, 0.75, axis=0)
    pmean = np.mean(variances1, axis=0)
    pstds = np.std(variances1, axis=0)

    smed = np.median(variances2, axis=0)
    sq1s = np.quantile(variances2, 0.25, axis=0)
    sq3s = np.quantile(variances2, 0.75, axis=0)
    smean = np.mean(variances2, axis=0)
    sstds = np.std(variances2, axis=0)

    plt.clf()
    plt.plot(nums, pmed, label="Penelope", color=penelope)
    plt.fill_between(nums, pq1s, pq3s, alpha=0.5, edgecolor=penelope, facecolor=penelope)

    plt.plot(nums, smed, label="SLURM", color=slurm)
    plt.fill_between(nums, sq1s, sq3s, alpha=0.5, edgecolor=slurm, facecolor=slurm)

    plt.xlabel("Number of Nodes")
    plt.ylabel("Variance in Power Transaction Size")
    plt.title("Variance of Power Transaction Size versus Scale")

    plt.legend()
    plt.savefig("variances.png")


if __name__ == '__main__':
    data_path = sys.argv[1]
    home = os.path.expanduser("~")
    path = home + "/" + data_path
    os.chdir(path)

    process_apps()
    graph()
