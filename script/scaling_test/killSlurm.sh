#!/bin/bash
server=$1

hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

for hostname in "${hostArray[@]}"
do
    ssh $hostname "touch /home/cc/END_SLURM" &
done

wait
sleep 5

for hostname in "${hostArray[@]}"
do
    ssh $hostname "rm /home/cc/END_SLURM" &
done

ssh $server "touch /home/cc/END_SLURM"
sleep 3
ssh $server "rm /home/cc/END_SLURM"
