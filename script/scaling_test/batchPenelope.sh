#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt

power=$1
scale_batch=$2

scale_batch() {
    power=$1
    fe_runtime=1200
    nums=(24 20 15 10 5 1)
    frequency=1000
    
    num_nodes=$(cat $hostfile | wc -l)
    for num in ${nums[@]}
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
}

frequency_batch () {
    power=$1
    fe_runtime=1200
    num=24
    freqs=(10 20 30 40 50 60 70 80 90 100 200 300 400 500 600 700 800 900 1000) 
    for f in ${freqs[@]} 
    do
        ./updateNumNodes.sh $num
        ./startPowerPool.sh $power $fe_runtime $f
        echo $num
        sleep 600
        ./stopPowerPool.sh
        ofd="penelope_scaling_$f"
        ./offload.sh $ofd
        ./resetEnv.sh
    done

}


if [ $scale_batch -eq 1 ] ;
then
    scale_batch $power
else
    frequency_batch $power
fi
