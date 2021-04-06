#!/bin/bash
frontendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/frontendHosts.txt))
backendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/backendHosts.txt))

level=D
apps=(bt cg ep ft is lu mg sp ua)

app1=$1
app2=$2
outfile1=$3
outfile2=$4

outdir="$app1"_"$app2"

for host in "${frontendHosts[@]}"
do
    ssh $host "/home/cc/penelope/run/NPB/run_npb_app.sh $app1 $outdir $level $outfile1" &
done

for host in "${backendHosts[@]}"
do
    ssh $host "/home/cc/penelope/run/NPB/run_npb_app.sh $app2 $outdir $level $outfile2" &
done

wait
