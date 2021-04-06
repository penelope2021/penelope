hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' /home/cc/penelope/script/hostnames_raw.txt))

for hostname in "${hostArray[@]}"
do
  ssh $hostname "pip install zmq" &
done

wait
