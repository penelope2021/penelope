#!/bin/bash

hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' hostnames_raw.txt))
i=0
for hostname in "${hostArray[@]}"
do
 ssh $hostname "sudo modprobe msr" & 
done

wait
