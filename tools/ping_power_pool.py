#
#   Hello World client in Python
#   Connects REQ socket to tcp://localhost:5555
#   Sends "Hello" to server, expects "World" back
#

import zmq
import time
import sys


context = zmq.Context()

#  Socket to talk to server
print("Connecting to hello world server...")
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:1111")
print("Sending start request")
socket.send_string("end")
message = socket.recv()
print("received message %s" % message)

# time.sleep(20)
# 
# print("Sending end request")
# socket.send(b"End")
