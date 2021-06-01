power=$1
runtime=$2
server=$3
num=$4
id=$5
frequency=$6

i=0
while [ $i -lt $num ]
do
    echo $i
    logname=/home/cc/slurm_output_"$power"_"$id"
    args="$power $runtime $server $id $frequency"
    echo "$args"
    python2 /home/cc/penelope/source/pmap_test/DistPwrClient.py $args > $logname &
    id=$((id+1))
    i=$((i+1))
done

wait
