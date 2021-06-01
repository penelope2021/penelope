#!/bin/bash

server=$1
power=$2

./batchSlurmFreq.sh $server $power

sleep 15

./batchPenelopeFreq.sh $power
