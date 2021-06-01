hostfile=/home/cc/penelope/script/hostnames_raw.txt
hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' $hostfile))

power=$1
f=$2
hostname=$3
num_nodes=$(cat $hostfile | wc -l)

logname=/home/cc/penelope_output_"$power"
sudo /home/cc/penelope/source/overhead_pool/power_pool $hostfile $num_nodes $power $hostname $f > $logname & 
# for hostname in "${hostArray[@]}"
# do
#     ssh $hostname "sudo /home/cc/penelope/source/overhead_pool/power_pool $hostfile $num_nodes $power $hostname $f > $logname" &
# done
