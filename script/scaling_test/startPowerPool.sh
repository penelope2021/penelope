hostfile=/home/cc/penelope/script/hostnames_scaling.txt
hostfile2=/home/cc/penelope/script/hostnames_raw.txt
#hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\:[0-9][0-9][0-9][0-9]' $hostfile))
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile2))
 
power=$1
fe_runtime=$2
frequency=$3

num_nodes=$(cat $hostfile | wc -l)
num_phys=$(cat $hostfile2 | wc -l)
mod_num=`expr $num_nodes / $num_phys`
echo $mod_num


id=0
i=0
for hostname in "${hostArray[@]}"
do
    runtime=300
    if [ $(( $i % 2 )) -eq 0 ]; then
        runtime=$fe_runtime
    fi
    args="$power $runtime $hostname $mod_num $id $frequency"
    ssh $hostname "/home/cc/penelope/script/scaling_test/deployPenelope.sh $args" &
    i=$(($i + 1))
    id=$(($id + $mod_num))
done
