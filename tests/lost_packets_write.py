#!/usr/bin/python

import socket
from time import sleep


DST_IP = "127.0.0.1"
LISTEN_PORT = 69
RAND_PORT = 5006

while True:
#if True:
    sock_start = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock_start.bind((DST_IP, LISTEN_PORT))

    data, addr = sock_start.recvfrom(1500)
    KLIENT_PORT = addr[1]

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((DST_IP, RAND_PORT))
    sleep(1.05)
    sock.sendto(str.encode("\0\4\0\0"), addr)
    sleep(0.05)
    sock.sendto(str.encode("\0\4\0\0"), addr)
    sleep(1.05)
    sock.sendto(str.encode("\0\4\0\1"), addr)
    sleep(0.05)
    sock.sendto(str.encode("\0\4\0\1"), addr)
    sleep(1.05)
    sock.sendto(str.encode("\0\4\0\2"), addr)
    sleep(0.05)
    sock.sendto(str.encode("\0\4\0\1"), addr)
    sleep(1.05)
    sock.sendto(str.encode("\0\4\0\3"), addr)