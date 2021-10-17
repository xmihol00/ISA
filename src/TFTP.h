#pragma once

using namespace std;

#include <memory>
#include "input.h"

#define TFTP_DATAGRAM_SIZE 516
#define TFTP_DATA_SIZE 512
#define RRQ  (1U << 8)
#define WRQ  (2U << 8)
#define DATA (3U << 8)
#define ACK  (4U << 8)
#define ERR  (5U << 8)
#define MAX_RETRIES 5U

void RRQ_header(char *buffer, const string &file_URL, data_mode_t mode, ssize_t &size);

void WRQ_header(char *buffer, const string &file_URL, data_mode_t mode, ssize_t &size);

void ACK_header(char *buffer, ssize_t &size);

string err_code_value(uint16_t err_code);
