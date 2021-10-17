#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <getopt.h>
#include <climits>
#include <cstring>

#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

#define DEFAULT_PORT 69U

enum transfer_mode_t
{
    READ = 1,
    WRITE = 2,
};

enum data_mode_t
{
    BINARY = 0,
    ASCII,
};

enum address_type_t
{
    UNSET = 0,
    IPv4,
    IPv6,
};

typedef struct 
{
    transfer_mode_t     transfer_mode;
    string              file_URL;
    uint32_t            timeout;
    uint32_t            block_size;
    data_mode_t         data_mode;
    address_type_t      address_type;
    union
    {
        in_addr ipv4;
        in6_addr ipv6;
    }                   address;
    in_port_t           port;
    bool                multicast;
} arguments_t;

void help_msg();

bool parse_line(const string &line, arguments_t &arguments);
