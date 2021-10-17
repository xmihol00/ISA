
#include "TFTP.h"

void RRQ_header(char *buffer, const string &file_URL, data_mode_t mode, ssize_t &size)
{
    size = file_URL.size();

    buffer[0] = (char)0;
    buffer[1] = (char)1;
    strcpy(&buffer[2], file_URL.c_str());

    if (mode == BINARY)
    {
        strcpy(&buffer[3 + size], "octet");
        size += 9;
    }
    else
    {
        strcpy(&buffer[3 + size], "netascii");
        size += 12;
    }
}

void ACK_header(char *buffer, ssize_t &size)
{
    buffer[0] = (char)0;
    buffer[1] = (char)4;
    buffer[4] = (char)0;
    size = 4;
}
