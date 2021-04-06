#!/bin/bash
# frontendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/frontendHosts.txt))
# frontend_hostname=$(head -n 1 ~/penelope/script/frontendHosts.txt)
# backendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/backendHosts.txt))
# backend_hostname=$(head -n 1 ~/penelope/script/backendHosts.txt)


level=D
apps=(bt cg ep ft lu mg sp ua)

rm -rf /home/cc/power_profile
rm -rf /home/cc/profile
mkdir /home/cc/power_profile
mkdir /home/cc/profile

basedir=/home/cc/NPB3.4.1/NPB3.4-OMP
cd $basedir

for app in ${apps[@]}
do
    mkdir -p ~/profile/$app

    for power in 60 70 80 90 100 110 120 130 140;
    do
        powerfile=/home/cc/power_profile/"$app"_rapl_"$power"

        sudo /home/cc/penelope/tools/RAPL/RaplSetPower $power $power  
        cmd="$app"."$level".x
        out=/home/cc/profile/$app/"$app"_"$level".out
        runtime_file=/home/cc/profile/$app/"$app"_"$level"_"$power"

        sudo rm -rf PowerResults.txt
        sudo /home/cc/penelope/tools/RAPL/RaplPowerMonitor_1s > /dev/null &
        start_time=`date +%s%3N`
        bin/$cmd > $out
        end_time=`date +%s%3N`
        sudo pkill Rapl

        runtime=$(($end_time - $start_time))
        echo $runtime > $runtime_file
        cp PowerResults.txt $powerfile

        sleep 5
    done
done
