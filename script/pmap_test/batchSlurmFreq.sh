#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt

server=$1
power=$2
fe_runtime=1200


num=24
fs=(0.01 0.05 0.1 0.5 0.75 1)
num_nodes=$(cat $hostfile | wc -l)

for frequency in ${fs[@]}
do
    echo $frequency
    ./updateNumNodes.sh $num
    ./launchScalingTest.sh $power $fe_runtime $server $num $frequency
    echo "finished"
    ./killSlurm.sh $server
    n=$(($num * $num_nodes))
    echo $n
    ofd=slurm_scaling_"$n"_"$frequency"
    echo $ofd
    ./offload.sh "$ofd"
    ./resetEnv.sh
    sudo pkill PollServer.py
    sudo pkill python
    sleep 10
done
