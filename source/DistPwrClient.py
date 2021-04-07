import os,sys,math,subprocess,time,socket,math
import datetime

powercap=int(float(sys.argv[1])) # initial cap
server_name=str(sys.argv[2]) # server ip
client_id=int(sys.argv[3]) 
id_list=[str(2*client_id), str(2*client_id +1)] # one client id per socket
server_address = (server_name, 10000)

cpu_list=[]
END_signal =0
frequency = 2
default_frequency=2
half_frequency=1
power_margin=3
half_power_margin=1
return_value=''
minimum_power_cap=30.0

LOG_FLAG = 1
cpu_files = []
if LOG_FLAG:
    LOG_file = open("/home/cc/slurm_output_"+ str(powercap), 'w')

cpu_files.append(open("/home/cc/slurm_core1_"+str(powercap)+".csv", 'w'))
cpu_files.append(open("/home/cc/slurm_core2_"+str(powercap)+".csv", 'w'))

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

class cpu:
    def __init__(self,id):
        self.id = id
        self.initial_power_limit=0.0
        self.current_power_limit=0.0
        self.next_power_limit=0.0
        self.current_power=0.0
        self.extra_power=0.0
        self.extra_power_with_margin=0.0
        self.priority = -1
        self.group = -1
        self.urgent = 0
        self.whichend=-1

#read last window average power
def ReadPower():
    FileName = "/home/cc/penelope/tools/RAPL/PowerResults.txt"
    Powerlist = open(FileName,'r').readlines()[-2:]
    print(Powerlist[-1])
    power1=0.0
    power2=0.0
    nowtime=0.0
    for i in range(len(Powerlist)):
        line = Powerlist[i].split()
        power1 += float(line[1])
        power2 += float(line[2])
        entry_time = float(line[0])
        if entry_time > nowtime:
            nowtime = entry_time

    power1 = power1/len(Powerlist)
    power2 = power2/len(Powerlist)
    return nowtime,power1,power2

# Assign node-level power cap
def SetPower(index,power):
    if index ==0:
        os.system("sudo /home/cc/penelope/tools/RAPL/RaplSetPower "+str(power)+" 0 >/dev/null")
    else:
        os.system("sudo /home/cc/penelope/tools/RAPL/RaplSetPower 0 "+str(power)+" >/dev/null")

def ExtraPower(current_power_limit, margin, current_power):
    if current_power <= minimum_power_cap:
        return int(current_power_limit - minimum_power_cap)
    else:
        return int(current_power_limit - current_power - margin * 0.5)

for i in range(2):
    cpu_tmp = cpu(i)
    cpu_tmp.initial_power_limit=powercap
    cpu_tmp.current_power_limit=powercap
    cpu_list.append(cpu_tmp)


#connect to the server
sock.connect(server_address)

while(END_signal !=1):
  #  os.system("sleep "+str(frequency))
    time.sleep(int(frequency))

    nowtime, cpu_list[0].current_power,cpu_list[1].current_power = ReadPower()
    #check if we have extra power
    actual_get_power,release_power=0,0
    for i in range(2):
        #sometimes the msr overflows, we need to filter out the bad power readings
        if cpu_list[i].current_power < 0 or cpu_list[i].current_power > 200:
            break
        actual_get_power,release_power=0,0
        printstr =  str(nowtime) + "," + str(cpu_list[i].current_power) + "," + str(cpu_list[i].current_power_limit) + "\n"
        cpu_files[i].write(printstr)
        cpu_files[i].flush()

        if cpu_list[i].current_power < cpu_list[i].current_power_limit - power_margin:
            #post extra power
            
            extra_power = ExtraPower(cpu_list[i].current_power_limit, power_margin, cpu_list[i].current_power)
            if extra_power != 0:
                cpu_list[i].urgent=0
            else:
                cpu_list[i].urgent=2
            
            cpu_list[i].next_power_limit = cpu_list[i].current_power_limit - extra_power
            SetPower(i,cpu_list[i].next_power_limit)
            cpu_list[i].current_power_limit = cpu_list[i].next_power_limit
            msg =str(extra_power)+','+str(cpu_list[i].urgent)+',' + id_list[i]+',0'
            if LOG_FLAG:
                LOG_file.write(str(datetime.datetime.now()) + ' message sent: extra power =' + str(extra_power) + ', urgent flag = 0, current_power = ' + str(cpu_list[i].current_power) + ', current_power_limit = ' + str(cpu_list[i].current_power_limit) + ', initial_power_limit = ' +  str(cpu_list[i].initial_power_limit) + '\n')
            try:
                sock.send(msg)
                return_value = sock.recv(1024)
                actual_get_power=float(return_value.split(',')[0])
                release_power=int(return_value.split(',')[1])
            except:
                sock.close()
                break
            
            if release_power == 1 and cpu_list[i].current_power_limit  > cpu_list[i].initial_power_limit:
                available_release_power= cpu_list[i].current_power_limit - cpu_list[i].initial_power_limit
                cpu_list[i].current_power_limit = cpu_list[i].initial_power_limit
                SetPower(i,cpu_list[i].current_power_limit)
                msg =str(available_release_power)+','+'0'+','+id_list[i]+',0'
                if LOG_FLAG:
                    LOG_file.write('Trying to release power: available_release_power = ' + str(available_release_power)+ '\n')
                try:
                    sock.send(msg)
                except:
                    sock.close()
                    break
                return_value = sock.recv(1024)
                
        else:
            #need power
            if cpu_list[i].current_power_limit >= cpu_list[i].initial_power_limit:
                #not necessary, check if need_power file is 0, if not get power from extra
                cpu_list[i].urgent=0
                msg ='0'+','+'0'+','+id_list[i]+',0'
                if LOG_FLAG:
                    LOG_file.write(str(datetime.datetime.now()) + ' message sent: extra power = 0, urgent flag = 0, current_power = ' + str(cpu_list[i].current_power) + ', current_power_limit = ' + str(cpu_list[i].current_power_limit) + ', initial_power_limit = ' +  str(cpu_list[i].initial_power_limit)+ '\n')
                
                try:
                    sock.send(msg)
                    return_value = sock.recv(1024)
                    actual_get_power=float(return_value.split(',')[0])
                    release_power=int(return_value.split(',')[1])
                except:
                    sock.close()
                    break

                if release_power == 1 :
                    available_release_power= cpu_list[i].current_power_limit - cpu_list[i].initial_power_limit
                    cpu_list[i].next_power_limit = cpu_list[i].current_power_limit -available_release_power
                    cpu_list[i].current_power_limit =cpu_list[i].next_power_limit 
                    SetPower(i,cpu_list[i].next_power_limit)
                    msg =str(available_release_power)+','+'0'+','+id_list[i]+',0'
                    #sock.send(msg)
                    if LOG_FLAG:
                        LOG_file.write('Trying to release power: available_release_power = ' + str(available_release_power)+ '\n')
                    try:
                        sock.send(msg)
                    except:
                        sock.close()
                        break
                    return_value = sock.recv(1024)
                else:
                    cpu_list[i].next_power_limit = cpu_list[i].current_power_limit + actual_get_power
                    SetPower(i,cpu_list[i].next_power_limit)
                    cpu_list[i].current_power_limit =cpu_list[i].next_power_limit

            #necessary
            else:
                necessary_power= cpu_list[i].initial_power_limit - cpu_list[i].current_power_limit
                cpu_list[i].urgent= 1
                msg ='0'+','+'1'+','+id_list[i] +','+ str(necessary_power)
                if LOG_FLAG:
                    LOG_file.write(str(datetime.datetime.now()) + ' message sent: extra power = 0, urgent flag = 1,necessary_power = ' + str(necessary_power) + ', current_power = ' + str(cpu_list[i].current_power) + ', current_power_limit = ' + str(cpu_list[i].current_power_limit) + ', initial_power_limit = ' +  str(cpu_list[i].initial_power_limit)+ '\n')
                try:
                    sock.send(msg)
                    return_value = sock.recv(1024)
                    actual_get_power,release_power=float(return_value.split(',')[0]),int(return_value.split(',')[1])
                except:
                    sock.close()
                    break

                if LOG_FLAG:
                    LOG_file.write('actual_get_power = ' + str(actual_get_power)+ '\n')
                if actual_get_power >=necessary_power:
                    cpu_list[i].urgent=0
                    
                if actual_get_power != 0:
                    cpu_list[i].next_power_limit = cpu_list[i].current_power_limit + actual_get_power
                    SetPower(i,cpu_list[i].next_power_limit)
                    cpu_list[i].current_power_limit =cpu_list[i].next_power_limit

    if cpu_list[0].urgent ==1 or cpu_list[1].urgent==1:
        frequency = half_frequency
    else:
        frequency = default_frequency
    if os.path.isfile('/home/cc/END_SLURM'):
        END_signal = 1

# clean up files and close socket
LOG_file.flush()
cpu_files[0].flush()
cpu_files[1].flush()

LOG_file.close()
cpu_files[0].close()
cpu_files[1].close()
sock.close()
