#pragma once

using namespace std;

#include "input.h"
#include "TFTP.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>

#define ACK_BUFFER_SIZE 1024U
#define MAX_IPv4_HDR 60
#define UDP_HDR 8
#define IPv6_HDR 40

typedef struct
{
    const string &file_URL;
    data_mode_t mode; 
    int32_t block_size;
    bool multicast;
    uint8_t timeout;
} transfer_data_t;

void transfer(const arguments_t &arguments);

void read(int socket_fd, struct sockaddr *address, socklen_t length, transfer_data_t data);

void write(int socket_fd, struct sockaddr *address, socklen_t length, transfer_data_t data);

int32_t get_MTU_of_used_if(sockaddr_in6 address, socklen_t lenght);

int32_t get_min_MTU(sockaddr destination);

void set_timeout(int socket_fd, uint8_t timeout);
