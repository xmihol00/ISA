#pragma once

using namespace std;

#include "input.h"
#include "TFTP.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <ifaddrs.h>

#define BUFFER_SIZE 1024U
#define ACK_BUFFER_SIZE 256U

void transfer(const arguments_t &arguments);

void read(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode);

void write(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode);

int get_MTU(int socket_fd);

int get_MTU(sockaddr destination, socklen_t len);
