#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <getopt.h>
#include <climits>
#include <cstring>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/statvfs.h>

using namespace std;

#define DEFAULT_PORT 69U
#define DEFAULT_TIMEOUT 1U

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
    uint8_t             timeout;
    int                 block_size;
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

string get_file_name(const string &file_URL);

long available_space();

void print_summary(const string &file, transfer_mode_t mode, int blksize, unsigned packet_count, long data_size, int lost, int lost_size, double time);
