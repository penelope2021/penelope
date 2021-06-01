from statistics import geometric_mean
import os
import sys

dirname = sys.argv[1]
if dirname[-1] != '/':
    dirname = dirname + '/'

apps = ["bt", "cg", "ep", "ft", "lu", "mg", "sp", "ua", "dc"]

home = os.path.expanduser("~")
basedir = home + "/uchicago_drive/npb_data/" + dirname

speedups = []
xs = []
for app in apps:
    appdir = basedir + str(app) + "/"
    appfile = str(app) + "_D_100"
    fair_filename     = appdir + "fair/" + appfile
    penelope_filename = appdir + "penelope/" + appfile

    fair_val = 0.0
    pen_val = 0.0
    with open(fair_filename) as f:
        line = f.readline().rstrip()
        fair_val = float(line)

    with open(penelope_filename) as f:
        line = f.readline().rstrip()
        pen_val = float(line)

    speedup = (pen_val - fair_val) / fair_val
    xs.append(app)
    speedups.append(speedup)

print(xs)
print(speedups)
avg = sum(speedups) / len(speedups)
print(avg)
# geo_xup = geometric_mean(speedups)
# print(geo_xup)


