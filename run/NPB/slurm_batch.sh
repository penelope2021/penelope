#!/bin/bash
server=$1
fault=${2:-0}
# apps=(bt cg ep ft is lu mg sp ua)
apps=(bt cg ep ft lu mg sp ua)
len=${#apps[@]}

ssh-keyscan -t rsa $server >> ~/.ssh/known_hosts

i=0
while [ $i -lt $len ]
do
    app1=${apps[$i]}

    j=$(($i+1))
    while [ $j -lt $len ]
    do
        app2=${apps[$j]}
        echo "$app1","$app2"
        for power in 60 70 80 90 100;
        do
            outfile1=slurm_"$app1"_"$power"
            outfile2=slurm_"$app2"_"$power"
            powerfile=slurm_"$power"

            /home/cc/penelope/script/startPowerMonitor.sh > /dev/null
            /home/cc/penelope/script/setPower.sh $power
        
            ./launch_cluster.sh $app1 $app2 $outfile1 $outfile2 &
            PID=$!
            /home/cc/penelope/script/launchSlurm.sh $server $power $fault
            wait $PID

            /home/cc/penelope/script/stopPowerMonitor.sh
            /home/cc/penelope/script/killSlurm.sh $server
            /home/cc/penelope/script/storePowerSeries.sh $app1 $app2 $powerfile
            sleep 10
            # don't do the harsh clean
            # /home/cc/penelope/script/clean.sh
            # sleep 5
        done

        j=$(($j+1))
    done
    i=$(($i+1))
done


