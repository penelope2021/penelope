import sys

n = int(sys.argv[1])
ports = [str(i) for i in range(1600, 1600+n)]

with open("/home/cc/penelope/script/hostnames_raw.txt") as f:
    of = open("/home/cc/penelope/script/hostnames_scaling.txt", "w")
    lines = [x.rstrip() for x in f.readlines()]
    hosts = [i+":"+j+"\n" for i in lines for j in ports]
    for h in hosts:
        of.write(h)
