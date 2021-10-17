#pragma once

using namespace std;

#include <memory>
#include "input.h"

void RRQ_header(char *buffer, const string &file_URL, data_mode_t mode, ssize_t &size);

void ACK_header(char *buffer, ssize_t &size);
