#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

for hostname in "${hostArray[@]}"
do
	# ssh $hostname "/home/cc/penelope/script/killPowerPool.sh"
    ssh $hostname "python /home/cc/penelope/tools/ping_power_pool.py" &
done

wait
