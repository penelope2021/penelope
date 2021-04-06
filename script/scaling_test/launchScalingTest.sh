#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_scaling.txt
hostfile2=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile2))

power=$1
fe_runtime=$2
server=$3
num=$4
frequency=$5

num_nodes=$(cat $hostfile | wc -l)
num_phys=$(cat $hostfile2 | wc -l)

# python2 ~/penelope/source/scaling_test/DistPwrServer.py $server $power > /dev/null 2> /dev/null &
# python2 ~/penelope/source/scaling_test/DistPwrServer.py $server $power > ~/slurm_server_output 2>&1 &
python2 ~/penelope/source/scaling_test/PollServer.py $server $power $num_nodes > ~/slurm_server_output 2>&1 &

sleep 5

id=0
i=0
for hostname in "${hostArray[@]}"
do
    runtime=300
    if [ $(( $i % 2 )) -eq 0 ]; then
        runtime=$fe_runtime
    fi
    args="$power $runtime $server $num $id $frequency"
    ssh $hostname "/home/cc/penelope/script/scaling_test/deploySlurm.sh $args" &
    i=$(($i + 1))
    id=$(($id + $num))
done
