hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' /home/cc/penelope/script/hostnames_raw.txt))

flag=$1
debug=""
if [ ! -z $flag ]
then
    debug="debug"
fi


for hostname in "${hostArray[@]}"
do
  ssh $hostname "cd /home/cc/penelope; git pull; cd source/power_pool; make clean && make $debug" &
done

wait
