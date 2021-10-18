#pragma once

using namespace std;

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <memory>
#include <unistd.h>
#include <string>
#include <algorithm>

#include "input.h"

#define TFTP_DATAGRAM_SIZE 516
#define TFTP_DATA_SIZE 512
#define TFTP_HDR 4
#define MAX_RETRIES 5U
#define OCTET "octet"
#define OCTET_LEN 6
#define NETASCII "netascii"
#define NETASCII_LEN 9
#define BLKSIZE "blksize"
#define BLKSIZE_LEN 8
#define TIMEOUT "timeout"
#define TIMEOUT_LEN 8
#define TSIZE "tsize"
#define TSIZE_LEN 6

enum opcode_t
{
    RRQ   = 1 << 8,
    WRQ   = 2 << 8,
    DATA  = 3 << 8,
    ACK   = 4 << 8,
    ERR   = 5 << 8,
    OACK  = 6 << 8,
};

enum err_code_t
{
    NOT_DEFINED =       0 << 8,
    FILE_NOT_FOUND =    1 << 8,
    ACCESS_VIOLATION =  2 << 8,
    DISK_FULL =         3 << 8,
    ILLEGAL_TFTP =      4 << 8,
    UNKNOWN_ID =        5 << 8,
    FILE_EXIST =        6 << 8,
    NO_USER =           7 << 8,
    BAD_OACK =          8 << 8,
};

typedef struct
{
    const string &file_URL;
    opcode_t opcode;
    data_mode_t mode;
    int block_size;
    long transfer_size;
    uint8_t timeout;
    bool multicast;
} TFTP_options_t;

typedef struct
{
    int block_size;
    long transfer_size;
    uint8_t timeout;
} negotiation_t;

void RQ_header(char *buffer, ssize_t &size, TFTP_options_t options);

void ACK_header(char *buffer, ssize_t &size, uint16_t ack_number);

void ERR_packet(char *buffer, ssize_t &size, err_code_t code, const char* message);

string err_code_value(uint16_t err_code);

negotiation_t parse_OACK(char *buffer, ssize_t size);
