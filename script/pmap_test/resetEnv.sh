#!/bin/bash
allHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/hostnames_raw.txt))

for host in "${allHosts[@]}"
do
    ssh $host "rm -f /home/cc/END_*; rm -rf /home/cc/trial; rm -f penelope_*; rm -f slurm_*" &
done

wait
