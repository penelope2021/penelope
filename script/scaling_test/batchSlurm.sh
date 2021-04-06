#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt

server=$1
power=$2
scale_batch=$3

scale_batch () {
    fe_runtime=1200
    nums=(48 44 40 36 30 24 20 10 5 1)
    num_nodes=$(cat $hostfile | wc -l)
    frequency=1

    for num in ${nums[@]}
    do
        echo $num
        ./updateNumNodes.sh $num
        ./launchScalingTest.sh $power $fe_runtime $server $num $f
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
}

frequency_batch () {
    power=$1
    fe_runtime=1200
    num=24
    freqs=(10 20 30 40 50 60 70 80 90 100 200 300 400 500 600 700 800 900 1000) 
    for f in ${freqs[@]} 
    do
        ./updateNumNodes.sh $num
        ./launchScalingTest.sh $power $fe_runtime $server $num $f
        sleep 600
        ./killSlurm.sh $server
        ofd="slurm_scaling_$f"
        ./offload.sh "$ofd"
        ./resetEnv.sh
        sudo pkill PollServer.py
        sudo pkill python
        sleep 10
    done
}

if [ $scale_batch -eq 1 ] ;
then
    scale_batch $power
else
    frequency_batch $power
fi

