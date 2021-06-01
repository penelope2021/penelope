#!/bin/bash
hostfile=/home/cc/penelope/script/hostnames_raw.txt
fefile=/home/cc/penelope/script/frontendHosts.txt
befile=/home/cc/penelope/script/backendHosts.txt
frontendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $fefile))
backendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $befile))

app1=$1
app2=$2
dir="$app1"_"$app2"

cd ~/penelope/script/pmap_test/power_profile/"$dir"


for hostname in "${frontendHosts[@]}"
do
    scp frontend.txt $hostname:/home/cc/powerfile
done

for hostname in "${backendHosts[@]}"
do
    scp backend.txt $hostname:/home/cc/powerfile
done

