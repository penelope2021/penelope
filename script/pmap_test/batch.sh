#!/bin/bash

server=$1
power=$2
apps=(bt cg ep ft lu mg sp ua dc)
len=${#apps[@]}
i=0
mkdir -p ~/scaling_test

while [ $i -lt $len ]
do
    app1=${apps[$i]}
    j=$(($i+1))
    while [ $j -lt $len ]
    do
        app2=${apps[$j]}
        echo "$app1"_"$app2"
        mkdir ~/scaling_test/"$app1"_"$app2"
        ./copy_pmap.sh $app1 $app2

        ./batchSlurm.sh $server $power
        sleep 15
        ./batchPenelope.sh $power
        mv ~/slurm_scaling_*    ~/scaling_test/"$app1"_"$app2" 
        mv ~/penelope_scaling_* ~/scaling_test/"$app1"_"$app2" 
        j=$(($j+1))
    done
    i=$(($i+1))
done
