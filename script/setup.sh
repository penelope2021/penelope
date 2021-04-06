#!/bin/bash

# create fe/be host files
# copy all files and ssh keys to whole cluster

server=$1
hostfile=/home/cc/penelope/script/hostnames_raw.txt
fefile=/home/cc/penelope/script/frontendHosts.txt
befile=/home/cc/penelope/script/backendHosts.txt

rm -f $fefile
rm -f $befile

hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

for hostname in "${hostArray[@]}"
do
    ssh-keyscan -t rsa $hostname >> ~/.ssh/known_hosts
    ssh $hostname "ssh-keyscan github.com >> ~/.ssh/known_hosts"
done

i=0
for hostname in "${hostArray[@]}"
do
    if [ $(( $i % 2 )) -eq 0 ]; then
        echo $hostname >> $fefile
    else
        echo $hostname >> $befile
    fi
    i=$(($i + 1))
done



for hostname in "${hostArray[@]}"
do
    scp $hostfile $hostname:/home/cc/penelope/script
    scp $fefile $hostname:/home/cc/penelope/script
    scp $befile $hostname:/home/cc/penelope/script
    scp /home/cc/.ssh/id_rsa $hostname:/home/cc/.ssh
    scp /home/cc/NPB3.4.1/NPB3.4-OMP/config/suite.def $hostname:/home/cc/NPB3.4.1/NPB3.4-OMP/config
    ssh $hostname "echo 0 | sudo tee /proc/sys/kernel/nmi_watchdog"
done

for hostname in "${hostArray[@]}"
do
    ssh $hostname "cd /home/cc/NPB3.4.1/NPB3.4-OMP/ && make suite" &
done
wait

echo "msr"
/home/cc/penelope/script/setmsr.sh
echo "end msr"

echo "launch keyscan"
/home/cc/penelope/script/launchKeyscanNodes.sh $server

echo "launch pcm"
/home/cc/penelope/script/makePCM.sh
echo "end pcm"

echo "make all"
/home/cc/penelope/script/pullCommit.sh debug
echo "end make"

echo "pip install zmq"
/home/cc/penelope/script/installZMQ.sh
echo "end install"






echo "setup complete"
echo "watch output to see if intermediate steps failed"

