'''
This client is modified for the scaling simultion. Rather than reading real
power, it always returns whatever its cap is. After a configurable amount of
time has elapsed, it mocks as idle, always returning that it consumes below 30W,
which is the minimum power cap.

We also add frequency as a configurable parameter. 
'''

import os,sys,math,subprocess,time,socket,math

powercap=int(float(sys.argv[1]))
end_time=float(sys.argv[2]) # time in seconds until decider begins "idling"
server_name=str(sys.argv[3]) # ip address
client_id=int(sys.argv[4])
frequency = float(sys.argv[5]) # period (in seconds) for decider iteration
id_list=[str(2*client_id), str(2*client_id +1)]
server_address = (server_name, 10000)

cpu_list=[]
END_signal =0
default_frequency = frequency
half_frequency = frequency
power_margin=3
half_power_margin=1
return_value=''
minimum_power_cap=30.0

# Every time we send a message, we log how long it took to receive a response
# we keep a running sum of these durations (because of the sheer number of
# request, keeping a list was becoming unweildy)
# we keep track of the number of messages, and compute the mean turnaround time
# at the end
turnaround_time = 0.0
num_msgs = 0
start_time = time.time()
outfile = "/home/cc/slurm_ttime_" + str(client_id)
ttime_file = open(outfile, 'w')

LOG_FLAG = 1

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

# ReadPower now returns either current cap or 26 (sufficiently below the 30W min
# cap to always be classified as having excess but never lowering below 30). 
# Returns a value based on how long it has been from launch
def ReadPower():
    nowtime = time.time()
    if nowtime - start_time > end_time:
        power1, power2 = 26, 26
    else:
        power1, power2 = cpu_list[0].current_power_limit, cpu_list[1].current_power_limit
    return nowtime, power1, power2

# Because these are simulated nodes, we cannot actualy make hardware changes. So
# we keep the API for minimal code difference but remove any actual power
# capping
def SetPower(index,power):
    return 

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
    time.sleep(frequency)

    nowtime, cpu_list[0].current_power,cpu_list[1].current_power = ReadPower()
    #check if we have extra power
    actual_get_power,release_power=0,0
    for i in range(2):
        #sometimes the msr overflows, we need to filter out the bad power readings
        if cpu_list[i].current_power < 0 or cpu_list[i].current_power > 200:
            break
        actual_get_power,release_power=0,0
        print nowtime,cpu_list[i].current_power,cpu_list[i].current_power_limit

        if cpu_list[i].current_power < cpu_list[i].current_power_limit - power_margin:
            #post extra power
            
            extra_power = ExtraPower(cpu_list[i].current_power_limit, power_margin, cpu_list[i].current_power)
            if extra_power != 0:
                cpu_list[i].urgent=0
            else:
                cpu_list[i].urgent=2
            
            cpu_list[i].next_power_limit = cpu_list[i].current_power_limit - extra_power
            SetPower(i,cpu_list[i].next_power_limit) #noop
            cpu_list[i].current_power_limit = cpu_list[i].next_power_limit
            msg =str(extra_power)+','+str(cpu_list[i].urgent)+',' + id_list[i]+',0'
            if LOG_FLAG:
                print(str(time.time()) + ' message sent: extra power =' + str(extra_power) + ', urgent flag = 0, current_power = ' + str(cpu_list[i].current_power) + ', current_power_limit = ' + str(cpu_list[i].current_power_limit) + ', initial_power_limit = ' +  str(cpu_list[i].initial_power_limit) + '\n')
            #sock.send(msg)
            try:
                start = time.time()
                sock.send(msg)
                return_value = sock.recv(1024)
                end = time.time()
                runtime = end - start
                print 'message turnaround time: ' + str(runtime)
                ttime_file.write(str(runtime) + '\n')
                turnaround_time += runtime
                num_msgs += 1

                actual_get_power=float(return_value.split(',')[0])
                release_power=int(return_value.split(',')[1])
            except:
                print("CLOSED AHHH DEATH")
                sock.close()
                break
            
            if release_power == 1 and cpu_list[i].current_power_limit  > cpu_list[i].initial_power_limit:
                available_release_power= cpu_list[i].current_power_limit - cpu_list[i].initial_power_limit
                cpu_list[i].current_power_limit = cpu_list[i].initial_power_limit
                SetPower(i,cpu_list[i].current_power_limit) # noop
                msg =str(available_release_power)+','+'0'+','+id_list[i]+',0'
                if LOG_FLAG:
                    print('Trying to release power: available_release_power = ' + str(available_release_power)+ '\n')

                start = time.time()
                try:
                    sock.send(msg)
                except:
                    print("CLOSED AHHH DEATH")
                    sock.close()
                    break
                return_value = sock.recv(1024)
                end = time.time()
                runtime = end - start
                print 'message turnaround time: ' + str(runtime)
                ttime_file.write(str(runtime) + '\n')
                turnaround_time += runtime
                num_msgs += 1
                
        else:
            #need power
            if cpu_list[i].current_power_limit >= cpu_list[i].initial_power_limit:
                #not necessary, check if need_power file is 0, if not get power from extra
                cpu_list[i].urgent=0
                msg ='0'+','+'0'+','+id_list[i]+',0'
                #sock.send(msg)
                if LOG_FLAG:
                    print(str(time.time()) + ' message sent: extra power = 0, urgent flag = 0, current_power = ' + str(cpu_list[i].current_power) + ', current_power_limit = ' + str(cpu_list[i].current_power_limit) + ', initial_power_limit = ' +  str(cpu_list[i].initial_power_limit)+ '\n')
                
                try:
                    start = time.time()
                    sock.send(msg)
                    return_value = sock.recv(1024)
                    end = time.time()
                    runtime = end - start
                    print 'message turnaround time: ' + str(runtime)
                    ttime_file.write(str(runtime) + '\n')
                    turnaround_time += runtime
                    num_msgs += 1
                    actual_get_power=float(return_value.split(',')[0])
                    release_power=int(return_value.split(',')[1])
                except:
                    print("CLOSED AHHH DEATH")
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
                        print('Trying to release power: available_release_power = ' + str(available_release_power)+ '\n')

                    start = time.time()
                    try:
                        sock.send(msg)
                    except:
                        print("CLOSED AHHH DEATH")
                        sock.close()
                        break
                    return_value = sock.recv(1024)
                    end = time.time()
                    runtime = end - start
                    print 'message turnaround time: ' + str(runtime)
                    ttime_file.write(str(runtime) + '\n')
                    turnaround_time += runtime
                    num_msgs += 1
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
                    print(str(time.time()) + ' message sent: extra power = 0, urgent flag = 1,necessary_power = ' + str(necessary_power) + ', current_power = ' + str(cpu_list[i].current_power) + ', current_power_limit = ' + str(cpu_list[i].current_power_limit) + ', initial_power_limit = ' +  str(cpu_list[i].initial_power_limit)+ '\n')
                try:
                    start = time.time()
                    sock.send(msg)
                    return_value = sock.recv(1024)
                    end = time.time()
                    runtime = end - start
                    print 'message turnaround time: ' + str(runtime)
                    ttime_file.write(str(runtime) + '\n')
                    turnaround_time += runtime
                    num_msgs += 1
                    actual_get_power,release_power=float(return_value.split(',')[0]),int(return_value.split(',')[1])
                except:
                    print("CLOSED AHHH DEATH")
                    sock.close()
                    break

                if LOG_FLAG:
                    print('actual_get_power = ' + str(actual_get_power)+ '\n')
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
    #if os.path.isfile('/mnt/signal/END'):
    if os.path.isfile('/home/cc/END_SLURM'):
        END_signal = 1

print 'Turnaround time total: ' + str(turnaround_time)


#print avg ttime to file, using client_id to prevent overwriting

# avg_ttime = turnaround_time / num_msgs
# outfile = "/home/cc/slurm_ttime_" + str(client_id)
# with open(outfile, 'w') as f:
#     f.write(str(avg_ttime))

#reset power cap and kill the power monitor
# os.system('sudo pkill rapl;sudo /home/cc/penelope/tools/RAPL/RaplSetPower 125 125 >/dev/null')
#runtime_info.close()
# LOG_file.flush()
# cpu_files[0].flush()
# cpu_files[1].flush()

# LOG_file.close()
# cpu_files[0].close()
# cpu_files[1].close()
sock.close()
