#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt

power=$1
fe_runtime=1200

nums=(1 7 15 20 24)
frequency=1000

num_nodes=$(cat $hostfile | wc -l)
for num in ${nums[@]}
do
    # python3 scalingHostFile.py $num
    ./updateNumNodes.sh $num
    ./startPowerPool.sh $power $fe_runtime $frequency
    echo $num
    # sleep 600
    # ./stopPowerPool.sh
    n=$(($num * $num_nodes))
    ofd=penelope_scaling_"$n"_"$frequency"
    ./offload.sh $ofd
    ./resetEnv.sh
done

