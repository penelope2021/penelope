
# Performance Under Nominal Conditions
## How to run SLURM
The server-ip is the internal server IP for the server node. The initial
powercap is self explanatory (initial cap per socket) and for each client assign
it a unique client-id, so the server can keep track of it. 
python2 DistPwrServer.py <server-ip>
python2 DistPwrClient.py <init-powercap> <server-ip> <client-id>

## How to run Penelope
Use the power\_pool directory. Run make. The executable, power\_pool, runs with
4 parameters: the path to the hostfile (with all the host IPs), the length of
that file, the initial powercap, and the ip for the node the executable is being
launched on.

./power\_pool <hostfile> <num-hosts> <init-powercap> <self-ip>

# Performance Under Faulty Conditions
## How to run SLURM
The client executable is exactly the same as the above case. For the server, we
use DistPwrServer.py and we add a second command line argument, the number of
iterations that the server runs for before terminating. We run it via
python2 DistPwrServerFault.py <server-ip> <num-iters>

Penelope and the SLURM local decider are unchanged.

# Scalability 
## How to run SLURM
We switch implementations for the server slightly to allow it to scale to a
higher number of clients. For both of these we use the scaling\_test directory.

We pass into the server the ip, the initial cap on each socket, and the nubmer
of clients it should expect.

We pass to the local decider the initial cap, how long it should run for before
mocking as though it is idling in seconds, the server ip, the client id, and how
frequently it iterates.

python2 PollServer.py <server-ip> <init-cap> <num-clients>
python2 DistPwrClient.py <init-cap> <endtime> <server-ip> <client-id> <frequency>

## How to run Penelope
We pass to Penelope the hostfile, number of hosts, initial cap, and self ip.
Since many instances are running in parallel, we also specify what port this
instance should listen on for its power pool. We pass in how long it spends
before "idling", it's id, and how frequently the local decider iterates.

./power\_pool <hostfile> <num-hosts> <init-powercap> <self-ip> <self-port> <runtime> <id> <frequency>

# Overhead
The overhead directory holds the source code for the modified version of
Penelope we use to test overhead. The only modifications made are to ensure no
edge case problems when trying to send messages because the cluster consists of
only 1 node.

We use a different executable that resides in the overhead\_pool directory, but
it is launched the same as the nominal and faulty cases in terms of command line
parameters.

./power\_pool <hostfile> <num-hosts> <init-powercap> <self-ip>
