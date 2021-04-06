frontendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/frontendHosts.txt))
backendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/backendHosts.txt))
allHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/hostnames_raw.txt))

dirname=$1

cd
mkdir -p $dirname
mv ~/power_profile $dirname
mv ~/out.txt $dirname
cd $dirname
mkdir -p frontend
mkdir -p backend


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

for host in "${allHosts[@]}"
do 
    ssh $host "rm /home/cc/END_SLURM" &
done

rm ~/END_SLURM
