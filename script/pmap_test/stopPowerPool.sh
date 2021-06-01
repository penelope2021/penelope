#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

for hostname in "${hostArray[@]}"
do
    ssh $hostname "touch /home/cc/END_PENELOPE" &
done

wait

sleep 5

for hostname in "${hostArray[@]}"
do
    ssh $hostname "rm /home/cc/END_PENELOPE" &
done

wait

