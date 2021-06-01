#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt

power=$1
fe_runtime=1200
num=24
fs=(10 50 100 500 750 1000)

num_nodes=$(cat $hostfile | wc -l)
for frequency in ${fs[@]}
do
    ./updateNumNodes.sh $num
    ./startPowerPool.sh $power $fe_runtime $frequency
    echo $num
    sleep 600
    ./stopPowerPool.sh
    n=$(($num * $num_nodes))
    ofd="penelope_scaling_$n"
    ./offload.sh $ofd
    ./resetEnv.sh
done

