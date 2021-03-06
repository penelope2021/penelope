#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt

server=$1
power=$2
fe_runtime=1200
num_nodes=$(cat $hostfile | wc -l)

nums=(1 5 10 15 20 24)
frequency=1

for num in ${nums[@]}
do
    echo $num
    ./updateNumNodes.sh $num
    ./launchScalingTest.sh $power $fe_runtime $server $num $frequency
    sleep 600
    ./killSlurm.sh $server
    n=$(($num * $num_nodes))
    echo $n
    ofd="slurm_scaling_$n"
    ./offload.sh "$ofd"
    ./resetEnv.sh
    sudo pkill PollServer.py
    sudo pkill python
    sleep 10
done
