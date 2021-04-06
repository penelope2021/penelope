hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/PowerShift/script/hostnames_raw.txt))

for hostname in "${hostArray[@]}";
do
	# ssh -o LogLevel=ERROR $hostname "grep /home/cc/PowerShift/tool/RAPL/PowerResults.txt -e '^100\.' | head -1 "
	ssh -o LogLevel=ERROR $hostname "tail -1 /home/cc/PowerShift/tool/RAPL/PowerResults.txt " &
    pids[${count}]=$!
    count=$(($count + 1))
done


for pid in ${pids[*]}
do
    wait $pid
done
