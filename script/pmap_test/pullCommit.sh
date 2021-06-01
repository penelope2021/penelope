hostArray=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' /home/cc/penelope/script/hostnames_raw.txt))

flag=$1
debug=""
if [ ! -z $flag ]
then
    debug="debug"
fi


for hostname in "${hostArray[@]}"
do
  ssh $hostname "cd /home/cc/penelope/source/pmap_test/power_pool; git pull; make clean && make $debug" &
done

wait
