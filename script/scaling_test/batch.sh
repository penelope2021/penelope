#!/bin/bash

server=$1
power=$2
scale_batch=$3

./batchSlurm.sh $server $power $scale_batch

sleep 15

./batchPenelope.sh $power $scale_batch
