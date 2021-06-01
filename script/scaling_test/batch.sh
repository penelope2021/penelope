#!/bin/bash

server=$1
power=$2

./batchSlurm.sh $server $power

sleep 15

./batchPenelope.sh $power
