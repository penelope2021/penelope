#!/bin/bash
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' /home/cc/penelope/script/hostnames_raw.txt))

for hostname in "${hostArray[@]}"
do
    sudo pkill launch_cluster.sh
    sudo pkill fair_batch.sh
    sudo pkill slurm_batch.sh
    sudo pkill penelope_batch.sh
    sudo pkill all_batch.sh
    ssh cc@$hostname "sudo pkill Rapl; sudo pkill python; sudo killall power_pool; sudo killall python; sudo pkill main" &
    ssh $hostname "/home/cc/penelope/script/killPowerPool.sh" &
    ssh $hostname "sudo pkill run_npb_app.sh" &
    ssh $hostname "sudo pkill 'D\.x'" &
done

wait
