#!/usr/bin/python

import socket
from time import sleep


DST_IP = "127.0.0.1"
LISTEN_PORT = 69
RAND_PORT = 5006

sock_start = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_start.bind((DST_IP, LISTEN_PORT))

while True:
#if True:
    data, addr = sock_start.recvfrom(1500)
    KLIENT_PORT = addr[1]

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((DST_IP, RAND_PORT))
    sock.sendto(str.encode("\0\6blksize\000200\000timeout\00010\000"), addr)
    
