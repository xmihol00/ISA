#pragma once

using namespace std;

#include "input.h"
#include "TFTP.h"

#include <unistd.h>

#define BUFFER_ZIZE 1024U

void transfer(const arguments_t &arguments);

void read(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode);

void write(const arguments_t &arguments, int socket_fd);
