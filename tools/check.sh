./pwrs.sh > powers.txt

x=$(awk '{s+=$2}END{print s}' powers.txt)
y=$(awk '{s+=$3}END{print s}' powers.txt)

echo "$x + $y" | bc | tee -a sumpowers.txt
