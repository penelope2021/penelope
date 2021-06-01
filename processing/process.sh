#!/bin/zsh

for d1 in */
do
    cd $d1
    echo $d1
    for d in */
    do
        echo $d
        cd $d
        mkdir ttime
        find . -name "*ttime*" -type f -exec cp "{}" ttime  \;
        cd ..
    done
    
    for d in penelope_*/
    do
        echo $d
        cd $d
        mkdir ctime
        find . -name "*ctime*" -type f -exec cp "{}" ctime  \;
        cd ..
    done
    cd ..
done


