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
files therein
.  
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
