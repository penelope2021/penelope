# penelope
Project Description
----------------------------------------
Penelope is a peer-to-peer power management system, which seeks to improve mean
application performance on large distributed clusters while enforcing
system-wide powercaps. Unlike existing centralized work, Penelope's distributed
design is robust in faulty environments or at large scale.

Dependency
----------------------------------------
A bare-metal cluster
Intel power capping technique RAPL support
ZMQ -- An open-source universal messaging library
pthread, bsd, GCC atomic builtins
Python2 -- Needed for the SLURM implementation

Penelope Details
----------------------------------------
Penelope's operation is logically separated from applications. Our experimental
setup involves launching an application and then simultaneously launching
Penelope. 

Penelope is implemented in C. The remaining files in this repository are
scripts that help quickly setup and launch experiments. 

Build: Penelope compiles with GCC. Makefiles are provided in each directory
containing C code to be compiled. 

Additional Notes:  
Unfortunately, this repository has been written in a way that is dependent on
the system for simplicity. For many scripts, paths must be modified. The core
Penelope executable should be portable, but the RAPL wrapper we use may need to
be modified based on the system platform.

Benchmarks
----------------------------------------
Penelope does not require any code instrumentation, so this repo does not
contain the individual workloads. We used the NAS Parallel
Benchmark version 3.4

Directory Structure  
----------------------------------------  
Please see READMEs in each subfolder for more details on that directory and
files therein.

├── run       -- Scripts to launch Penelope alongside benchmark on the cluster  
├── tool      -- RAPL wrapper and some other utilities  
├── source    -- Source files  
├── scripts   -- Scripts used to setup our cluster  
└── data      -- Data collected  

How to Run Penelope
----------------------------------------  
Our experiments were run on a Chameleon testbed (you can find out more at
chameleoncloud.org). In order to test all 3 systems, you need 1 more node than
you intend to run workloads on to support SLURM's central server. We use this
node also as the primary point from which we run all batch scripts.

We configured each node at launch to accept an ssh-key for ssh access. First,
copy that ssh-key as .ssh/id\_rsa on the server node. All the operations occur
within this git repository. As such, first enter the repo on the server node and
run git pull to make sure you have the latest changes.

Next, copy the list of IP
addresses for every node in the system (excluding the server node) to
scripts/hostnames\_raw.txt. We want to allow the cluster time to warm up so that
we can actually communicate between nodes. As such, you can go into the scripts
directory and run ./keyscanNodes.sh <server-ip>. You can get <server-ip> from
ifconfig or the like, but be sure to use the internal, not public IP.

This script simply attempts to use the ssh-keyscan tool on all the IPs listed in
hostnames\_raw.txt, so it can be repeated or interrupted without major problem.
Once that script is able to keyscan every node, run setup.sh <server-ip> from
the scripts directory. This will run all the necessary steps to install packages
and otherwise setup each node in the system. 

To run the real benchmarks or overhead see the README in the run directory. To run the
scaling tests, go the scaling\_test subdirectory under scripts. To run Penelope
or SLURM directly, for either real tests or scaling tests, see the source
directory.


Reproducing Resuts
----------------------------------------
There are a few caveats that must be noted before running these steps. First,
these scripts assume that current user is "cc" and that the home directory is in
"/home/cc". Second, it assumes that in the home directory there is a directory
"NPB3.4.1" within which is the NPB parallel benchmark (you can download the
precise version we use here
https://www.nas.nasa.gov/assets/npb/NPB3.4.1.tar.gz).  Additionally, within the
OMP subdirectory, you will need to set the suite configuration file suite.def
(there is a template in the repo) and set all levels to D. You will also need to
add a row for "dt" and set the level for that to B since "dt" doesn't compile
above level B. 

A final caveat: while I aimed to make these scripts general, in practice I
launched these scripts from the server node. I would place a public IP on this
node, meaning that compute nodes are only accessible internally. As a result, it
is possible that some scripts were written based on the assumption that the
initially running node is the server. So I would suggest following this to avoid
potential unintended complications.

## Setup
These are some common setup steps (which are are discussed above). Any setup
specific to experiments is discussed under that heading.

1. Copy the ssh-key needed to access chameleon nodes to .ssh/id\_rsa. 
2. Create script/hostnames\_raw.txt, which is a newline separated file that
   contains the IPs for every compute node (NOT the server node) in the cluster.
3. Run "git pull" within the repo to get the most up-to-date code
4. Run "script/keyscanNodes.sh SERVER-IP" (SERVER-IP is of course the internal
   IP of the server node, the only IP not included in hostnames\_raw.txt). This
   just warms up the cluster. Keep running this until the script is able to
   keyscan all nodes in the cluster.
5. Make sure that (a) the NPB benchmark is in the home directory on every
   compute node and (b) that there is a suite.def file created on the server
   node (from which all setup scripts are being run)
6. Run "script/setup.sh SERVER-IP" 

## Experiments 
1. Performance under nominal conditions

Once everything is ready, the benchmark is compiled on all nodes, and all other
setup is completed, you should run a file "all\_batch" within "run/NPB".
all\_batch.sh takes two parameters: a server IP and an integer. In our cluster,
as noted above, 1 node was withheld as a server node. So if you have a 21
node cluster like we used, there are 20 compute nodes and 1 server. The local IP
(not public IP) of the server is the first parameter to this script. The second,
for the experiment under nominal conditions, should be 0. So ./all\_batch.sh
SERVER-IP 0 would run this experiment. All data would be collected and put into
a directory on the node from which this script was run, which I would again
suggest be the server node.

2. Performance under failing conditions

This is very analogous to above: we run the same script but vary the second
parameter. The second parameter to all\_batch.sh represents the number of
iterations that the SLURM server should run before exiting. We ran with the
value 1770, since empirically this worked out to about a few minutes of uptime
before a server failure. 

Running: ./all\_batch SERVER-IP 1770 would reproduce this experiment. 

As a final note: I have written a script for each system--1
for Fair, a static allocation method, SLURM a centralized power management
system, and Penelope our proposed peer-to-peer power management system. If you
wish to run one system on its own, without also running the others, you can
reference (in run/NPB) fair\_batch.sh, slurm\_batch.sh, or penelope\_batch.sh.


For the next two experiments all scripts to run are within the "script"
subdirectory. 

3. Performance at scale (simulation)

NOTE: this experiment was actually replaced with experiment 4 below based on
reviews. However, I leave the code in the repo and provide instructions here on
how to run that experiment. 

All scripts reside within "script/scaling\_test".  This experiment created
infinitely power hungry nodes (source visible in source/scaling\_test) and then
released power into the system, timing how quickly that power was redistributed
and the average network turnaround time. 

There are two experiments: one fixes local decider frequency at 1 request/second
and varies simulated scale. The second fixes scale at the largest simulatable
and varies frequency (increasing past 1 request/second). To run the first test,
you can run ./batch.sh SERVER-IP INIT-POWERCAP. SERVER-IP is again the internal
IP of the server, and INIT-POWERCAP is the initial cap assigned to the simulated
nodes. In our experiment this was 100. To run the second test, run
./batchFreq.sh SERVER-IP INIT-POWERCAP (all arguments are the same). So to
run the same experiments run the following commands:

./batch.sh SERVER-IP 100
./batchFreq.sh SERVER-IP 100

4. Performance at scale (simulations using power maps)

This experiment uses power profiles collected on the NPB applications at low
scale (experiments 1 and 2). These profiles are in the power\_profile directory
within script/pmap\_test. The script "copy\_pmap.sh" handles distributing the
correct applications profile to the correct nodes. 

Like experiment 3, this experiment has two parts: one which varies scale and
fixes frequency, and one which varies frequency and fixes scale. To run the
former, run batch.sh. For the latter, use batchFreq.sh. The arguments are the
same as in experiment 3. The first is the server IP address, the second is the
initial powercap (which we set to 100 for all trials). So executing the following
would run these experiments:

./batch.sh SERVER-IP 100
./batchFreq.sh SERVER-IP 100


Post-Processing
----------------------------------------  
There are two scripts that are needed for processing experiments 1 and 2. In the
processing subdirectory, there is max.py, which takes a path to data. This path does not
include the home directory. For example, if data is in a directory
"/home/cc/penelope\_data" one simply must run "python3 max.py penelope\_data".
This condenses the data into a CSV. 

Once that processing is done, use "python3 logs.py -p -s geo penelope\_data" to
graph this as a bar plot and get the plots used in the first two experiments in
the figure. "logs.py" graphs many other values that were useful to diagnostics
during research but were not included in the paper. 

Within this directory are scripts entitled ctime\_scale.py, ttime\_scale.py,
ctime\_freq.py, ttime\_freq.py, and var.py. These scripts process the pmap
tests. I have not included scripts to process the scaling tests, although they
are quite similar, because those graphs are not in the paper at the moment. 

Scripts ending in "freq" are designed to be run on data produced when varying
decider frequency. Scripts ending in "scale" are designed for those varying
scale. These 5 scripts all take a PATH. This is the location of data. 

The batch scripts described in the experiments above put all data in a directory
called "scaling\_test". Rename this accordingly. Then pass in the path to it,
again modulo the home directory. So if the directory was
"/home/cc/scaling\_test", simply pass in "scaling\_test" to these scripts. It is
best to keep frequency data in a separate directory. I tend to have one called
"frequency\_test" which holds all data when I vary frequency, and vice versa for
"scaling\_test". 

ctime\_freq/scale.py processes the redistribution time for experiments. It plots
the median and max redistribution time for both Penelope and SLURM.
ttime\_freq/scale.py does the same for average turnaround time. var.py plots the
variance of power transaction size versus scale. var.py is not configured to run
on frequency data, although can be easily adapted to do so.
