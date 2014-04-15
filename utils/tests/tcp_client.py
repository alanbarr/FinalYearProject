#!/usr/bin/env python3

import socket
import time

TCP_IP = "127.0.0.1"
TCP_PORT = 9000
BUFFER_SIZE = 1024
MESSAGE =                       \
"GET /pressure HTTP/1.0\r\n"    \
"Host: localhost\r\n"           \
"User-Agent: C\r\n"             \
"\r\n";

MESSAGE =                       \
"POST /device_name/pressure HTTP/1.0\r\n"   \
"Host: localhost\r\n"           \
"Content-Length: 7\r\n"         \
"\r\n"                          \
"123 kPa";

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
s.send(MESSAGE.encode("ascii"))
data = s.recv(BUFFER_SIZE)
s.close()
time.sleep(1)

print("received data:", data.decode())
