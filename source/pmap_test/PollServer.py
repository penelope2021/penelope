#!/bin/python2
import select
import socket
import sys,os
import Queue
import math
import time,datetime
total_extra_power=0
# 96 sockets in total
urgent_list= [0] * 10000
urgent_count=0
take_power_unit=2
break_signal = 0

server_ip = str(sys.argv[1])
power = int(sys.argv[2])
num_clients = int(sys.argv[3])

DEBUG_FLAG = 1
LOG_FLAG = 1


start_flag = False
conv_flag = False
quar1_flag = False
quar2_flag = False
quar3_flag = False


start_time = 0
end_time = 0
beginning = time.time()

max_power = num_clients * (power - 30)
quartile1 = 0.25*max_power
quartile2 = 0.5*max_power
quartile3 = 0.75*max_power
power_given = 0.0
power_received = 0.0

request_runtime = 0
num_reqs = 0
responding_time = 0
num_responses = 0

if LOG_FLAG:
    LOG_file = open("/home/cc/penelope/data/slurm/PoolLog/"+ str(int(beginning)), 'w')

READ_ONLY = select.POLLIN | select.POLLPRI | select.POLLHUP | select.POLLERR
READ_WRITE = READ_ONLY | select.POLLOUT
pool_file = open("/home/cc/slurm_pool_" + str(power), "w")

def take_power_func(TOTAL_extra_power, clients):
    x = math.floor(TOTAL_extra_power * 0.1 / clients)
    power = max(x, 1)
    return power

# Create a TCP/IP socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setblocking(0)

# Bind the socket to the port
server_address = (server_ip, 10000)
if DEBUG_FLAG:
    print >>sys.stderr, 'starting up on %s port %s' % server_address
    LOG_file.write('Power Pool starting at the time:' + str(datetime.datetime.now()) + '\n')
server.bind(server_address)

# Listen for incoming connections
server.listen(50)

# Outgoing message queues (socket:Queue)
message_queues = {}

# convert fd returned by poll to socket
fd_to_socket = {server.fileno(): server, }
poller = select.poll()
poller.register(server, READ_ONLY)
# num_clients = 0

prev_iter = time.time()
while break_signal != 1:
    if os.path.isfile('/home/cc/END_SLURM'):
        break_signal = 1
        if LOG_FLAG:
            LOG_file.write('Power Pool shutting down...')

    # Wait for at least one of the sockets to be ready for processing
    if DEBUG_FLAG:
        print >>sys.stderr, '\nwaiting for the next event'

    cur = time.time() - beginning
    foo = str(cur) + "," + str(total_extra_power) + "\n"
    pool_file.write(foo)

    events = poller.poll(1000)
    print("num events: " + str(len(events)))
    print("time for events: " + str(time.time() - prev_iter))
    prev_iter = time.time()
    for fd, flag in events:
        s = fd_to_socket[fd]
        if flag & (select.POLLIN | select.POLLPRI):

            if not start_flag and total_extra_power > 0:
                start_flag = True
                start_time = time.time()
                print('start: ' + str(start_time) + ' ' + str(total_extra_power))

            if not quar1_flag and start_flag and power_given > quartile1:
                quar1_time = time.time()
                runtime = quar1_time - start_time
                print('quar1_time, runtime: ' + str(quar1_time) + ' ' + str(runtime) + ' ' +
                        str(power_given))
                quar1_flag = True
                with open("/home/cc/slurm_ctime_"+str(power), 'a') as f:
                    f.write(str(runtime) + '\n')

            if not quar2_flag and start_flag and power_given > quartile2:
                quar2_time = time.time()
                runtime = quar2_time - start_time
                print('quar2_time, runtime: ' + str(quar2_time) + ' ' + str(runtime) + ' ' +
                        str(power_given))
                quar2_flag = True
                with open("/home/cc/slurm_ctime_"+str(power), 'a') as f:
                    f.write(str(runtime) + '\n')

            if not quar3_flag and start_flag and power_given > quartile3:
                quar3_time = time.time()
                runtime = quar3_time - start_time
                print('quar3_time, runtime: ' + str(quar3_time) + ' ' + str(runtime) + ' ' +
                        str(power_given))
                quar3_flag = True
                with open("/home/cc/slurm_ctime_"+str(power), 'a') as f:
                    f.write(str(runtime) + '\n')

            if not conv_flag and start_flag and power_given >= max_power and total_extra_power == 0:
                end_time = time.time()
                runtime = end_time - start_time
                print('end, runtime: ' + str(end_time) + ' ' + str(runtime) + ' ' +
                        str(power_given))
                conv_flag = True
                with open("/home/cc/slurm_ctime_"+str(power), 'a') as f:
                    f.write(str(runtime))
            # if conv_flag and start_flag and total_extra_power > 0:
            #     print('resetting ' + str(total_extra_power))
            #     conv_flag = False

            # if not conv_flag and start_flag and total_extra_power == 0:
            #     pass


            if s is server:
                # A "readable" server socket is ready to accept a connection
                connection, client_address = s.accept()
                if DEBUG_FLAG:
                    print >>sys.stderr, 'new connection from', client_address
                connection.setblocking(0)
                fd_to_socket[ connection.fileno() ] = connection
                poller.register(connection, READ_ONLY)

                # Give the connection a queue for data we want to send
                message_queues[connection] = Queue.Queue()
                # num_clients += 1

            else:
                data = s.recv(1024)
                if data:
                    st = time.time()
                    # A readable client socket has data
                    receive_list = data.split(',')
                    add_power =float(receive_list[0])
                    urgent_flag = int(receive_list[1])
                    #urgent_change = int(receive_list[2])
                    client_id = int(receive_list[2])
                    necessary_power =float(receive_list[3])
                    take_power_unit = take_power_func(total_extra_power,
                            num_clients)
                    actual_get_power = 0
                    release_power = 0
                    urgent_count = sum(urgent_list)

                    if add_power != 0:
                        total_extra_power +=add_power
                        power_received += add_power
                        urgent_list[client_id] = 0
                    else:
                        if urgent_flag ==1:
                            #urgent case
                            actual_get_power = min(total_extra_power, necessary_power)
                            urgent_list[client_id] = 1
                            if total_extra_power >= necessary_power:
                                urgent_list[client_id] = 0
                            total_extra_power = total_extra_power - actual_get_power
                            power_given += actual_get_power
                        elif urgent_flag == 2:
                            #stable
                            actual_get_power = 0
                            urgent_list[client_id] =0
                        else:
                            urgent_list[client_id] =0
                            if urgent_count ==0:
                                actual_get_power = min(take_power_unit, total_extra_power)
                                total_extra_power -= actual_get_power
                                power_given += actual_get_power
                    
                    if urgent_count != 0:
                        release_power = 1
                    if DEBUG_FLAG:
                        print 'received_data',data,'urgent_count',urgent_count,'total_extra_power',total_extra_power

                    output_data= str(actual_get_power) +','+str(release_power)
                    message_queues[s].put(output_data)
                    if LOG_FLAG:
                        LOG_file.write('Received: add_power = ' + str(add_power) +
                                        ', urgent_flag = ' + str(urgent_flag) + 
                                        ', necessary_power = ' + str(necessary_power) + '\n')
                        LOG_file.write('Sent: actual_get_power = '+ str(actual_get_power) +
                                        ', release_power = '+str(release_power) + '\n')
                        LOG_file.write('Now total power in power pool: ' + str(total_extra_power) + '\n')

                    # Add output channel for response
                    poller.modify(s, READ_WRITE)
                    et = time.time()
                    request_runtime += et - st
                    num_reqs += 1

                else:
                    # Interpret empty result as closed connection
                    if DEBUG_FLAG:
                        print >>sys.stderr, 'closing', client_address, 'after reading no data'
                    # Stop listening for input on the connection
                    poller.unregister(s)
                    s.close()

                    # Remove message queue
                    del message_queues[s]

        elif flag & select.POLLHUP:
            # Client hung up
            print >>sys.stderr, 'closing', client_address, 'after receiving HUP'
            # Stop listening for input on the connection
            poller.unregister(s)
            s.close()
        elif flag & select.POLLOUT: #sending 
            # Socket is ready to send data, if there is any to send.
            st = time.time()
            try:
                next_msg = message_queues[s].get_nowait()
            except Queue.Empty:
                # No messages waiting so stop checking for writability.
                print >>sys.stderr, 'output queue for', s.getpeername(), 'is empty'
                poller.modify(s, READ_ONLY)
            else:
                print >>sys.stderr, 'sending "%s" to %s' % (next_msg, s.getpeername())
                s.send(next_msg)
            et = time.time()
            responding_time += (et - st)
            num_responses += 1

        elif flag & select.POLLERR: #exceptional
            print >>sys.stderr, 'handling exceptional condition for', s.getpeername()
            # Stop listening for input on the connection
            poller.unregister(s)
            s.close()

            # Remove message queue
            del message_queues[s]
server.close()
print("power_received: " + str(power_received))
print("power_given: " + str(power_given))
print("max_power: " + str(max_power))
print("request_runtime: " + str(request_runtime))
print("num_reqs: " + str(num_reqs))
print("responding_time: " + str(responding_time))
print("num_responses: " + str(num_responses))

