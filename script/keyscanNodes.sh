#!/bin/bash

server=$1
hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

for hostname in "${hostArray[@]}"
do
    ssh-keyscan -t rsa $hostname >> ~/.ssh/known_hosts
done
ssh-keyscan -t rsa master >> ~/.ssh/known_hosts # will print error if not using spark
ssh-keyscan -t rsa github.com >> ~/.ssh/known_hosts
ssh-keyscan -t rsa $server >> ~/.ssh/known_hosts

