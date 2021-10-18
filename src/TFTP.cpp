
#include "TFTP.h"

void RRQ_header(char *buffer, const string &file_URL, data_mode_t mode, ssize_t &size)
{
    uint16_t *opcode = (uint16_t *)buffer;
    size = file_URL.size();

    *opcode = RRQ;
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

void WRQ_header(char *buffer, const string &file_URL, data_mode_t mode, ssize_t &size)
{
    uint16_t *opcode = (uint16_t *)buffer;
    size = file_URL.size();

    *opcode = WRQ;
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

void ACK_header(char *buffer, uint16_t ack_number, ssize_t &size)
{
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];

    *opcode = ACK;
    *block_number = ack_number;
    buffer[4] = (char)0;
    size = 4;
}

string err_code_value(uint16_t err_code)
{
    switch (ntohs(err_code))
    {
        case 0:
            return "(0) Not defined, see error message (if any).";

        case 1:
            return "(1) File not found.";

        case 2:
            return "(2) Access violation.";
        
        case 3:
            return "(3) Disk full or allocation exceeded.";
        
        case 4:
            return "(4) Illegal TFTP operation.";
        
        case 5:
            return "(5) Unknown transfer ID.";
        
        case 6:
            return "(6) File already exists.";
        
        case 7:
            return "(7) No such user.";
    
        default:
            return "Uknonw error code.";
    }
}
