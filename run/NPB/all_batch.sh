#!/bin/bash
server=$1 # server IP
fault=${2:-0} # number of iterations to run during fault case. pass in 0 for standard test, defaults to standard test if no input provided

rm -rf /home/cc/power_profile
mkdir -p /home/cc/power_profile

echo "fair---------------------------------"
/home/cc/penelope/run/NPB/fair_batch.sh
echo "slurm--------------------------------"
/home/cc/penelope/run/NPB/slurm_batch.sh $server $fault
echo "penelope-----------------------------"
/home/cc/penelope/run/NPB/penelope_batch.sh
