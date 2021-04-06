#!/bin/bash
server=$1
power=$2
fault=${3:-0}
echo $fault

hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))
# iteration=4

if [ $fault -eq 0 ] 
then
    ssh $server "rm -f /home/cc/END_SLURM; python2 /home/cc/penelope/source/DistPwrServer.py $server >/dev/null 2>&1" &
else
    ssh $server "rm -f /home/cc/END_SLURM; python2 /home/cc/penelope/source/DistPwrServerFault.py $server $fault >/dev/null 2>&1" &
    # ssh $server "rm -f /home/cc/END_SLURM; python2 /home/cc/penelope/source/DistPwrServerFault.py $server $fault" &
fi

sleep 2

i=0
for hostname in "${hostArray[@]}"
do
    ssh $hostname "rm -f /home/cc/END_SLURM; python2 /home/cc/penelope/source/DistPwrClient.py $power $server $i > slurm_log_"$power"" &
    i=$(($i+1))
done


