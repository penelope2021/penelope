#!/bin/bash

server=$1
hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

# worst case this is redundant. best case, allows it to be run from 
# a node not in the cluster (i.e. spark master)
# rm /home/cc/.ssh/known_hosts; /home/cc/penelope/script/keyscanNodes.sh

for hostname in "${hostArray[@]}"
do
    ssh $hostname "/home/cc/penelope/script/keyscanNodes.sh $server" &
done

wait
