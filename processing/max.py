'''
Create final_data.csv for each experiment based on the last node to complete its
workload. In essence, take the max of runtimes for every node that is the
runtime for a certain system. Construct a csv file for each app pair of the
runtimes of all 3 systems at all 5 power levels. This is then easily read by
logs.py to graph into a bar plot 
'''

import sys
import os
import itertools

systems = ["fair", "penelope", "slurm"]
powercaps = ["60", "70", "80", "90", "100"]
apps = ["bt", "cg", "ep", "ft", "lu", "mg", "sp", "ua", "dc"]
pairs = [(x,y) for x in apps for y in apps if x != y]

dir_name = sys.argv[1]
if dir_name[-1] != '/':
    dir_name = dir_name + '/'

home = os.path.expanduser("~")
basepath = home + "/" + dir_name
outdir = basepath + "final_data/"
os.makedirs(outdir, exist_ok=True)

for pair in itertools.combinations(apps, 2):
    app1, app2 = pair
    app_dir = app1 + "_" + app2 + "/"
    app_outdir = outdir + app_dir
    os.makedirs(app_outdir, exist_ok=True)
    outstrs = []
    outfile = app_outdir + "final_data.csv"

    for cap in powercaps:
        perf = {"fair":0, "penelope": 0, "slurm":0}    
        for system in systems:
            data_path1 = basepath + "frontend/"
            data_path2 = basepath + "backend/"

            max_data = 0
            for data_folder in os.listdir(data_path1):
                basename = system + "_" + app1 + "_" + cap
                filename = data_path1 + data_folder + "/" + app_dir + basename
                with open(filename, "r") as f:
                    line = f.readline().strip()
                    if float(line) > max_data:
                        max_data = float(line)

            for data_folder in os.listdir(data_path2):
                basename = system + "_" + app2 + "_" + cap
                filename = data_path2 + data_folder + "/" + app_dir + basename
                with open(filename, "r") as f:
                    line = f.readline().strip()
                    if float(line) > max_data:
                        max_data = float(line)
 
            perf[system] = max_data

        strperf = str(perf["fair"]) + "," + str(perf["slurm"]) + "," + str(perf["penelope"]) + "\n"
        outstrs.append(strperf)

    with open(outfile, "w") as f:
        f.write("".join(outstrs))
