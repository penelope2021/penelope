#!/bin/bash
app=$1
outdir=$2
level=$3
outfile=$4

basedir=/home/cc/NPB3.4.1/NPB3.4-OMP/
cd $basedir

cmd="$app"."$level".x

# assuming that all prior trial folders are deleted prior to running a new trial
mkdir -p ~/trial
mkdir -p ~/trial/"$outdir"
mkdir -p ~/trial/app_output
mkdir -p ~/trial/app_output/"$outdir"

out=/home/cc/trial/app_output/"$outdir"/"$outfile"

if [ "$app" == "dc" ] 
then 
    cmd="$app".B.x
    start_time=`date +%s%3N`
    bin/$cmd > $out
    bin/$cmd > $out
    bin/$cmd > $out
    end_time=`date +%s%3N`
else
    start_time=`date +%s%3N`
    bin/$cmd > $out
    end_time=`date +%s%3N`
fi

runtime=$(($end_time - $start_time))
echo $runtime > /home/cc/trial/"$outdir"/"$outfile"

mv /home/cc/penelope_* /home/cc/trial/"$outdir"
mv /home/cc/slurm_* /home/cc/trial/"$outdir"
