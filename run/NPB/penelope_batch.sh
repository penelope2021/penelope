#!/bin/bash

# apps=(bt cg ep ft is lu mg sp ua)
apps=(bt cg ep ft lu mg sp ua)
len=${#apps[@]}
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
            outfile1=penelope_"$app1"_"$power"
            outfile2=penelope_"$app2"_"$power"
            powerfile=penelope_"$power"

            /home/cc/penelope/script/setPower.sh $power
        
            ./launch_cluster.sh $app1 $app2 $outfile1 $outfile2 &
            PID=$!
            /home/cc/penelope/script/startPowerPool.sh $power
            wait $PID
            # /home/cc/penelope/script/clean.sh
            /home/cc/penelope/script/stopPowerPool.sh
            /home/cc/penelope/script/storePowerSeries.sh $app1 $app2 $powerfile
            sleep 10
        done

        j=$(($j+1))
    done
    i=$(($i+1))
done

