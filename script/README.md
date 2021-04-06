To setup the cluster:
1) Copy ssh-key onto server node (to see what "server" means, refer below)
2) Create hostnames\_raw.txt w/in script. It should list all IPs separated by
newlines, excluding server node's IP
3) Not strictly necessary but likely necessary. Run script/keyscanNodes.sh until
the server node is able to keyscan all the nodes in the system.
4) Run script/setup.sh SERVER\_IP, where SERVER\_IP is the private (i.e. not
floating) IP of the server node. 

The entire setup is contained within the setup.sh script. This script handles
copying the proper files to all nodes, ensuring dependencies are met, and that
each node has the most up to date version of the source code. There are really
only a few things that must be done manually.

Typically, if I intend to run Penelope on N nodes, I will reserver N+1 (in order
to evaluate SLURM) and attach the floating IP (chameleon terminology) to that
node. For the rest of this document, I'll refer to this node as the "server"
node. Penelope will not utilize this node, but slurm will. 

The first thing to do is copy the ssh key onto the server node (copying to the
rest of the cluster is handled by scripts). I organized it so that each node,
when launched, is accessible via this ssh key, and this ssh key also enables
access to the git repo for penelope, ensuring that only one (1) ssh key is 
necessary for a node to access everything it needs to. 

After copying the ssh key, you should pull the git repo on the server node,
adding it to your known hosts when prompted. Next you need to make a file called
"hostnames\_raw.txt" in the script subdirectory within the penelope repo. This
file should have a list of IP addresses for all the nodes in the system
(excluding the server node), separated by newlines. The setup script will divide
these into frontend and backend sets, with each set responsible for running an
application. 

Then all you (should) need to do is run setup.sh and provide as a command line
argument the server node's private IP. This means not floating, but the internal
chameleon IP.

In practice what you should do is run script/keyscanNodes.sh until that script
is able to keyscan all the nodes on the list. You might get errors like node
unreachable. This is usually due to connections not being warmed up yet on
chameleon. Run this script a few times until it runs totally cleanly, then
launch setup.sh.

Additional Notes/Tips:

I have gained some wisdom doing this. Perhaps I can help others.

a) I never liked adding executables, the hostname files, or data files to the
repo. Be careful when running git add ., since you'll add them. It's annoying

b) There is a clean.sh script, that will, by design, absolutely nuke any running
process on any node. As a result, I don't call it from scripts intended to run
applications. But, if something goes wrong, that's your big red button.


An overview of each script in this directory:
- clean.sh: as noted above, aims to kill any possible process that could have
  been launched. 
- installZMQ.sh: installs ZMQ on each client node. called from setup.sh
- keyscanNodes.sh: local script. keyscans all nodes from hostnames\_raw.txt
- killPowerPool.sh: local script that force kills Penelope on that node
- killSlurm.sh: Script that gracefully ends SLURM on all nodes and the server
- launchKeyscanNodes.sh: runs the keyscanNodes.sh script on every node in the cluster
- launchSlurm.sh: launches SLURM and local decider on every node and also launches the server
- offload.sh: Gathers logs from each node in the system and copies them to one directory on the server node
- pullCommit.sh: Pulls the most recent git commit on each node, cleans the
  Penelope build and remakes. Takes an argument that specifies whether to
  recompile Penelope with the verbose flag
- resetEnv.sh: Deletes all previous logs from clients and removes any files used
  to gracefully end SLURM or Penelope.
- setPower.sh: Sets power statically on every node. Takes powercap as parameter
- setmsr.sh: Opens the MSR on each node. Needed for RAPL to function. Run from
  setup.sh
- setup.sh: Sets up the cluster.
- startPowerMonitor.sh: Starts a RAPL instance on every node. Needed for SLURM
- startPowerPool.sh: Starts Penelope on every node. 
- stopPowerMonitor.sh: Kills RAPL instance started via startPowerMonitor.sh
- stopPowerPool.sh: Gracefully stops the power pool
- storePowerSeries.sh: Stores output of RAPL prints.

# Scaling Test
To run the scaling test we need some more steps. The scaling test operates by
running multiple clients simultaneously on one node. Make sure not to exceed the
max number of cores on your processor, otherwise you could cause unpredictable
performance/preemption issues.

After you run the setup steps
dictated earlier, you run updateNumNodes.sh <num-nodes>, where you pass as a
parameter the number of simulated nodes you would like to run on every physical
node. So if you want 30 simulated clients per physical node, pass in 30. The
total number of simulated nodes is then num-nodes times num-physical-nodes.

There are two types of scaling tests: ones that vary scale, and ones that vary
frequency but fix scale. the batch scripts all take a flag indicating whether
they should be batching scale or frequencies. Please, before running, check the
batchPenelope.sh and batchSlurm.sh scripts to make sure that the values being
looped over are as desired.
























