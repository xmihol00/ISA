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
    sock.sendto(str.encode("\0\3\0\001512 characters --------------------------------------------------------------\n??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????\n##############################################################################\n--------------------------------------------------\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*********\n"), addr)
    sleep(0.05)
    sock_start.sendto(str.encode("\0\3\0\2Wrong PORT"), addr)
    sleep(0.05)
    sock.sendto(str.encode("\0\3\0\002512 characters --------------------------------------------------------------\n??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????\n##############################################################################\n--------------------------------------------------\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*********\n"), addr)
    sleep(0.05)
    sock_start.sendto(str.encode("\0\3\0\3Wrong PORT"), addr)
    sock.sendto(str.encode("\0\3\0\3Konec souboru.\n"), addr)

