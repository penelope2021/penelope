frontendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/frontendHosts.txt))
backendHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/backendHosts.txt))
allHosts=($(grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' ~/penelope/script/hostnames_raw.txt))

app1=$1
app2=$2
filename=$3

mkdir -p ~/power_profile
mkdir -p ~/power_profile/"$app1"_"$app2"
#mkdir -p ~/power_profile/"$app2"

for hostname in "${frontendHosts[@]}"
do
    scp $hostname:/home/cc/penelope/tools/RAPL/PowerResults.txt /home/cc/
    mv ~/PowerResults.txt ~/power_profile/"$app1"_"$app2"/"$hostname"_"$filename"
done

for hostname in "${backendHosts[@]}"
do
    scp $hostname:/home/cc/penelope/tools/RAPL/PowerResults.txt /home/cc/
    mv ~/PowerResults.txt ~/power_profile/"$app1"_"$app2"/"$hostname"_"$filename"
done

