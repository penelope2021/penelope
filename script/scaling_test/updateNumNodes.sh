hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/hostnames_raw.txt))
num=$1

python3 scalingHostFile.py $num

for hostname in "${hostArray[@]}"
do
    scp ~/penelope/script/hostnames_scaling.txt $hostname:/home/cc/penelope/script &
done

wait

