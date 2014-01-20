#! /usr/bin/env python3
# Expected to receive a message from the CC3000 with: "Hello World from CC3000"
# It then responds with a "Hello CC3000".

import socket

UDP_IP = "10.0.0.1"
#UDP_IP = "127.0.0.1"
UDP_PORT = 44444

MSG_EXP = "Hello World from CC3000"
MSG_EXP_BYTES = MSG_EXP.encode()

MSG_TX = "Hello CC3000"
MSG_TX_BYTES = MSG_TX.encode()

print("Creating socket...")
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
print("Created!")

print("Binding to:", UDP_IP, ":", UDP_PORT)
sock.bind((UDP_IP, UDP_PORT))
print("Bound!")

while True:
    data_bytes, (src_ip, src_port) = sock.recvfrom(256)

    data = data_bytes.decode()
    
    print("Message Received:")
    print("data is: ", data)
    print("src_ip is: ", src_ip)
    print("src_port is`: ", src_port)

    if data != MSG_EXP:
        print("Message text was not as expected.")
        continue
    else:
        print("Sending Reply...")
        sock.sendto(MSG_TX_BYTES, (src_ip, src_port))
        print("Sent!")


