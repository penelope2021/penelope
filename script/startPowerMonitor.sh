hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' /home/cc/penelope/script/hostnames_raw.txt))

for hostname in "${hostArray[@]}"
do
  ssh $hostname "sudo pkill Rapl;cd /home/cc/penelope/tools/RAPL; sudo rm -f PowerResults.txt; sudo ./RaplPowerMonitor_1s &" &
done

