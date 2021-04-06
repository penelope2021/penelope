hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

power=$1
num_nodes=$(cat $hostfile | wc -l)

logname=/home/cc/penelope_output_"$power"
# start=$(date +"%s")
# logname="$start"
for hostname in "${hostArray[@]}"
do
    ssh $hostname "sudo /home/cc/penelope/source/power_pool/power_pool $hostfile $num_nodes $power $hostname > $logname" &
done
