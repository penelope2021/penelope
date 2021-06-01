allHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/hostnames_raw.txt))
apps=(bt cg ft lu sp ua)


i=0
for host in "${allHosts[@]}"
do
    if [ "$i" -eq 6 ]
    then 
        app="ep mg dc"
    else
        app=${apps[$i]}
    fi
    ssh $host "/home/cc/penelope/run/NPB/overhead/overhead_test.sh $host $app" &
    i=$(($i+1))
done

wait

for host in "${allHosts[@]}"
do
    scp -r $host:/home/cc/overhead .    
done
