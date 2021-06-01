'''
Script to graph various statistics
'''

import os
from os import path
import sys
import itertools
from statistics import geometric_mean
import argparse
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

apps = ["bt", "cg", "ep", "ft", "lu", "mg", "sp", "ua", "dc"]

'''
Graph data as a bar chart 

arr is the array of data
length is how many bars to plot: either 5 or 6 (with or without geo mean)
title is the title of the graph
outfile is the file to write the final image to
'''
def graph_runtime(arr, length, title, outfile):
    ind = np.arange(length)
    width = 0.3

    penelope='#a22a2b'
    slurm='#357e99'
    fig, ax = plt.subplots()
    rects2 = ax.bar(ind, arr[:,1], width, color=slurm, label='Slurm')
    rects3 = ax.bar(ind + width, arr[:,2], width, color=penelope, label='Penelope')
    
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Performance')
    ax.set_xticks(ind + width/2)
    ax.set_title(title)
    ax.set_xlabel('Powercap per socket per Node')

    if length == 5:
        ax.set_xticklabels(('60W', '70W', '80W', '90W', '100W'))
    elif length == 6:
        ax.set_xticklabels(('60W', '70W', '80W', '90W', '100W', 'Geo. Mean'))

    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    
    def autolabel(rects):
        """
        Attach a text label above each bar displaying its height
        """
        for rect in rects:
            height = rect.get_height()
            ax.text(rect.get_x() + rect.get_width()/2., 1.01*height,
                    '{:.2f}'.format(height),
                    ha='center', va='bottom', fontsize=6)
    
    autolabel(rects2)
    autolabel(rects3)
    plt.tight_layout()
    plt.savefig(outfile, facecolor=fig.get_facecolor(),
            edgecolor='#d5d4c2')
    plt.close()

'''
Normalize data and plot barchart

filename is the input file
outfilename is the filename to write the final plot to
'''
def process_runtime(filename, outfilename):
    final_data = np.genfromtxt(filename, delimiter=',')
    normalized = np.reciprocal(final_data) / np.reciprocal(final_data[:,0][:,np.newaxis])
    graph_runtime(normalized, 5, 'Total', outfilename)


def batch_runtime(dirname):
    home = os.path.expanduser("~")
    basedir = home + "/" + dirname
    baseoutdir = basedir + "graphs/"
    os.makedirs(baseoutdir, exist_ok=True)
    
    for pair in itertools.combinations(apps, 2):
        app1, app2 = pair
        app_dir = app1 + "_" + app2 + "/"
        filename = basedir + "final_data/" + app_dir + "final_data.csv"
        baseoutdir_app = baseoutdir + app_dir
        os.makedirs(baseoutdir_app, exist_ok=True)
        outfilename = baseoutdir_app + app1 + "_" + app2 + "_runtime.png"

        process_runtime(filename, outfilename)

def geometric_mean_graph(dirname):
    home = os.path.expanduser("~")
    basedir = home + "/" + dirname
    baseoutdir = basedir + "graphs/"
    os.makedirs(baseoutdir, exist_ok=True)
    
    slurm_perfs = {x:[] for x in range(5)}
    penelope_perfs = {x:[] for x in range(5)}

    for pair in itertools.combinations(apps, 2):
        app1, app2 = pair
        app_dir = app1 + "_" + app2 + "/"
        filename = basedir + "final_data/" + app_dir + "final_data.csv"
        baseoutdir_app = baseoutdir + app_dir
        os.makedirs(baseoutdir_app, exist_ok=True)
        outfilename = baseoutdir_app + app1 + "_" + app2 + "_runtime.png"

        final_data = np.genfromtxt(filename, delimiter=',')
        normalized = np.reciprocal(final_data) / np.reciprocal(final_data[:,0][:,np.newaxis])
        for i in range(5):
            slurm_perfs[i].append(normalized[i][1])
            penelope_perfs[i].append(normalized[i][2])

    geo_means = [] 
    for i in range(5):
        geo_slurm    = geometric_mean(slurm_perfs[i])
        geo_penelope = geometric_mean(penelope_perfs[i])
        geo_means.append([1, geo_slurm, geo_penelope])
    
    data_to_graph = np.array(geo_means)

    slurm_mean = np.mean(data_to_graph[:,1])
    penelope_mean = np.mean(data_to_graph[:,2])
    means = np.array([1,slurm_mean, penelope_mean])
    data_to_graph = np.vstack([data_to_graph, means])

    outfilename = baseoutdir + "geo_mean_runtime.png"
    graph_runtime(data_to_graph, 6, 'Performance Under Faulty Conditions' + 
            ' Normalized to Fair', outfilename)

# pass in dircnt/corecnt for graph file naming
def graph_stat(dirname, dir_, core,  corecnt, index, powercap, statname):
    penelope_color = '#a22a2b'
    slurm_color = '#357e99'
    colors = [slurm_color, penelope_color]
    for f in core:
        filename = dirname + dir_ + f
        if not path.exists(filename):
            continue

        data = np.genfromtxt(filename, delimiter=',')
        if data.size != 0:
            if "penelope" in f:
                plt.plot(data[:,0], data[:,index], color=colors[1], label = "Penelope") 
            else:
                plt.plot(data[:,0], data[:,index], color=colors[0], label = "Slurm") 

    plt.title(statname.capitalize() + " versus Time")
    plt.legend()
    outfile = dirname + dir_ + statname + "_core" + str(corecnt) + "_" + powercap +".png"
    plt.savefig(outfile)
    print(outfile)
    plt.close()

# pass in array of systems, and powercap interested in
def generate_files(systems, powercap):
    files = []
    for i in range(1,3):
        core_files = []
        for system in systems:
            add_str = system + "_core" + str(i) + "_" + powercap + ".csv"
            core_files.append(add_str)
        files.append(core_files)
    return files

# needs to read files, graph proper data, and write to outfile
# pass in systems list, directory, column of csv, and stat name
def process_stat(systems, dirname, index, statname):
    powercaps = ["60", "70", "80", "90", "100"]
    sub_clusters = ["frontend/", "backend/"]

    for sc in sub_clusters:
        data_path = dirname + sc
        for data_folder in os.listdir(data_path):
            for pair in itertools.combinations(apps, 2):
                app1, app2 = pair
                app_dir = app1 + "_" + app2 + "/"
                sub_folder = data_folder + "/" + app_dir
                for powercap in powercaps:
                    files = generate_files(systems, powercap)
                    corecnt = 1
                    for core in files:
                        graph_stat(data_path, sub_folder, core, corecnt, index, powercap, statname)
                        corecnt += 1

def main():
    # arg parsing
    parser = argparse.ArgumentParser()
    parser.add_argument("-P", "--presentation", help="Set background to that for presentation", action="store_true")
    parser.add_argument("-p", "--penelope", help="Use penelope files", action="store_true")
    parser.add_argument("-s", "--slurm", help="Use slurm files", action="store_true")
    parser.add_argument("stat", help="What graph to build: options are IPC," + 
            "power, powercap, runtime, geo, or all. IPC is likely not supported," + 
            "and will not be run with all")
    parser.add_argument("path", help="path to data")
    args = parser.parse_args()

    # ensure that dirname ends with / so we can append to it
    dirname = args.dir
    if dirname[-1] != '/':
        dirname = dirname + '/'

    # define system constants so user(me) doesn't need to write the whole path
    home = os.path.expanduser("~")
    basedir = home + "/" + dirname
    baseoutdir = basedir + "graphs/"

    systems = []
    if args.penelope:
        systems.append("penelope")
    if args.slurm and args.stat != "ipc":
        systems.append("slurm")


    # need to be negative since we need to count backwards
    # because Tapan is bad at planning the ordering of columns

    if args.stat == "all":
        geometric_mean_graph(dirname)
        batch_runtime(dirname)
        stat_map = {"powercap":-1, "power":-2}
        for stat in stats:
            statoutdir = baseoutdir + stat + "/"
            os.makedirs(baseoutdir, exist_ok=True)
            os.makedirs(statoutdir, exist_ok=True)
            process_stat(systems, basedir, stat_map[stat], stat)
    else:
        index = 0
        if args.stat == "ipc":
            index = -3
        elif args.stat == "power":
            index = -2
        elif args.stat == "powercap":
            index = -1
        elif args.stat == "runtime":
            batch_runtime(dirname)
            return
        elif args.stat == "geo":
            geometric_mean_graph(dirname)
            return
        else:
            exit("invalid stat name: options are ipc, power, powercap, runtime")

        baseoutdir = basedir + "graphs/"
        statoutdir = baseoutdir + args.stat + "/"
        os.makedirs(baseoutdir, exist_ok=True)
        os.makedirs(statoutdir, exist_ok=True)

        process_stat(systems, basedir, index, args.stat)

if __name__ == '__main__':
    main()
