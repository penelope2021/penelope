#!/bin/bash
# frontendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/frontendHosts.txt))
# frontend_hostname=$(head -n 1 ~/penelope/script/frontendHosts.txt)
# backendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/backendHosts.txt))
# backend_hostname=$(head -n 1 ~/penelope/script/backendHosts.txt)


level=D
hostname=$1
shift
a=$@
apps=($a)
# apps=(bt cg ep ft lu mg sp ua dc)

rm -rf /home/cc/power_profile
# rm -rf /home/cc/overhead
mkdir /home/cc/power_profile
# mkdir /home/cc/overhead

basedir=/home/cc/NPB3.4.1/NPB3.4-OMP
cd $basedir
power=100

for app in ${apps[@]}
do
    mkdir -p ~/overhead/$app/penelope
    for f in 10 50 100 250 500 750 1000;
    do
        sudo /home/cc/penelope/tools/RAPL/RaplSetPower $power $power  
        cmd="$app"."$level".x
        out=/home/cc/overhead/"$app"/penelope/"$app"_"$level".out
        runtime_file=/home/cc/overhead/$app/penelope/"$app"_"$level"_"$f"

        start_time=`date +%s%3N`
        bin/$cmd > $out &
        PID=$!
        /home/cc/penelope/run/NPB/overhead/startPowerPool.sh $power $f $hostname
        wait $PID
        end_time=`date +%s%3N`
        # /home/cc/penelope/script/stopPowerPool.sh
        python /home/cc/penelope/tools/ping_power_pool.py
        mkdir ~/overhead/"$app"/penelope/"$f"
        mv ~/penelope_* ~/overhead/"$app"/penelope/"$f"

        runtime=$(($end_time - $start_time))
        echo $runtime > $runtime_file
        sleep 5
    done
done

for app in ${apps[@]}
do
    mkdir -p ~/overhead/$app/fair
    sudo /home/cc/penelope/tools/RAPL/RaplSetPower $power $power  
    cmd="$app"."$level".x
    out=/home/cc/overhead/"$app"/fair/"$app"_"$level".out
    runtime_file=/home/cc/overhead/"$app"/fair/"$app"_"$level"_"$power"

    start_time=`date +%s%3N`
    bin/$cmd > $out
    end_time=`date +%s%3N`

    runtime=$(($end_time - $start_time))
    echo $runtime > $runtime_file
    sleep 5
done



