#pragma once

using namespace std;

#include "input.h"
#include "TFTP.h"

#include <unistd.h>

#define BUFFER_ZIZE 1024U
#define ACK_BUFFER_SIZE 256U

void transfer(const arguments_t &arguments);

void read(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode);

void write(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode);
