#! /usr/bin/env python3

import socket
import time

#UDP_IP = "127.0.0.1"
UDP_IP = "10.0.0.1"
UDP_PORT = 34444

MSG = "Hello World from CC3000"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
    sock.sendto(MSG.encode(), (UDP_IP, UDP_PORT))

    time.sleep(1)

