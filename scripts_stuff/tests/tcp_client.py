#!/usr/bin/env python

import socket
import time


TCP_IP = '10.0.0.10'
TCP_PORT = 80
BUFFER_SIZE = 1024
MESSAGE =                       \
"GET /pressure HTTP/1.0\r\n"    \
"Host: localhost\r\n"           \
"User-Agent: C\r\n"             \
"\r\n";

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
s.send(MESSAGE)
data = s.recv(BUFFER_SIZE)
s.close()
time.sleep(1)

print "received data:", data
