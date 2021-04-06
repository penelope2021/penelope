# Run directory

Main Scripts
------------------
This directory houses scripts specific to running workloads from the NAS
Parallel Benchmark. These scripts operate hierarchically. all\_batch.sh simply
calls the individual batch scripts for each system. Each system's script (i.e.
fair\_batch.sh, etc.) sets up that system and then launches the workload.
launch\_cluster.sh runs one application on half the cluster and runs a second
application on the other half. run\_npb\_app.sh runs on each client node and
actually runs the application on that node. profile\_benchmark.sh runs each
workload individually under different static caps. We used this to get an idea
of what the behavior of each application was under different static caps. 

Overhead
-------------------
The overhead directory holds the two scripts needed to launch the overhead test.
The first iterates over all applications, chooses an initial powercap, and runs
the benchmark under fair static allocation and then with Penelope to observe
overhead-related performance loss. The startPowerPool.sh script simply runs the
proper executable from the source directory. Look at the README in that
directory to get more information.
