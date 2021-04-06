frontendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/frontendHosts.txt))
backendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/backendHosts.txt))
allHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/hostnames_raw.txt))

dirname=$1

cd
mkdir -p ~/$dirname
mv ~/slurm_ctime_* ~/$dirname
mv ~/slurm_server_output ~/$dirname
mv ~/slurm_pool_* ~/$dirname

cd $dirname
mkdir -p frontend
mkdir -p backend

for host in "${allHosts[@]}"
do 
    ssh $host "mkdir /home/cc/trial && mv /home/cc/slurm_* /home/cc/penelope_* /home/cc/trial" &
done

wait

cd frontend
for host in "${frontendHosts[@]}"
do
    scp -r $host:/home/cc/trial $host
done

cd ../backend
for host in "${backendHosts[@]}"
do
    scp -r $host:/home/cc/trial $host
done

# for host in "${allHosts[@]}"
# do 
#     ssh $host "rm /home/cc/END_PENELOPE" &
# done

