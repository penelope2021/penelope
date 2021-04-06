#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_scaling.txt
num_nodes=$(cat $hostfile | wc -l)

power=$1
runtime=$2
hostname=$3
num=$4
id=$5
frequency=$6

echo "hi $frequency"

i=0
port=1600
while [ $i -lt $num ]
do
    echo $i
    logname=/home/cc/penelope_output_"$power"_"$id"
    args="$hostfile $num_nodes $power $hostname $port $runtime $id $frequency"
    echo $args
    sudo /home/cc/penelope/source/scaling_test/power_pool/power_pool $args > $logname &
    id=$((id+1))
    i=$((i+1))
    port=$(($port + 1))
done


