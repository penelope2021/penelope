hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/hostnames_raw.txt))
power=$1

for hostname in "${hostArray[@]}"
do
  ssh $hostname "sudo modprobe msr; sudo /home/cc/penelope/tools/RAPL/RaplSetPower $power $power" &
done

wait
