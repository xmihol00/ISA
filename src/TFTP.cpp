
#include "TFTP.h"

void RQ_header(char *buffer, ssize_t &size, TFTP_options_t options)
{
    string value;
    uint16_t *opcode = (uint16_t *)buffer;
    size = options.file_URL.size() + 3;

    *opcode = options.opcode;
    strcpy(&buffer[2], options.file_URL.c_str());

    if (options.mode == BINARY)
    {
        strcpy(&buffer[size], OCTET);
        size += OCTET_LEN;
    }
    else
    {
        strcpy(&buffer[size], NETASCII);
        size += NETASCII_LEN;
    }

    strcpy(&buffer[size], BLKSIZE);
    size += BLKSIZE_LEN;
    value = to_string(options.block_size);
    strcpy(&buffer[size], value.c_str());
    size += value.size() + 1;

    if (options.transfer_size >= 0)
    {
        strcpy(&buffer[size], TSIZE);
        size += TSIZE_LEN;
        value = to_string(options.transfer_size);
        strcpy(&buffer[size], value.c_str());
        size += value.size() + 1;
    }

    if (options.timeout != 0)
    {
        strcpy(&buffer[size], TIMEOUT);
        size += TIMEOUT_LEN;
        value = to_string(options.timeout);
        strcpy(&buffer[size], value.c_str());
        size += value.size() + 1;
    }
}

void ACK_header(char *buffer, ssize_t &size, uint16_t ack_number)
{
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];

    *opcode = ACK;
    *block_number = ack_number;
    buffer[4] = (char)0;
    size = 4;
}

negotiation_t parse_OACK(char *buffer, ssize_t size)
{
    negotiation_t negotiation { .block_size = -1, .transfer_size = -1, .timeout = 0 };
    ssize_t done = 2;
    string option;
    string value;

    while (done < size)
    {
        option = string(&buffer[done]);
        done += option.size() + 1;
        for_each(option.begin(), option.end(), [](char & c) 
        {
            c = tolower(c);
        });

        value = string(&buffer[done]);
        done += value.size() + 1;

        if (option == BLKSIZE)
        {
            negotiation.block_size = stoi(value);
        }
        else if (option == TIMEOUT)
        {
            negotiation.timeout = stoul(value);
        }
        else if (option == TSIZE)
        {
            negotiation.transfer_size = stol(value);
        }    
    }

    return negotiation;
}

string err_code_value(uint16_t err_code)
{
    switch (err_code)
    {
        case NOT_DEFINED:
            return "(0) Not defined, see error message (if any).";

        case FILE_NOT_FOUND:
            return "(1) File not found.";

        case ACCESS_VIOLATION:
            return "(2) Access violation.";
        
        case DISK_FULL:
            return "(3) Disk full or allocation exceeded.";
        
        case ILLEGAL_TFTP:
            return "(4) Illegal TFTP operation.";
        
        case UNKNOWN_ID:
            return "(5) Unknown transfer ID.";
        
        case FILE_EXIST:
            return "(6) File already exists.";
        
        case NO_USER:
            return "(7) No such user.";
        
        case BAD_OACK:
            return "(8) Usupported option.";
    
        default:
            return "Uknonw error code.";
    }
}

void ERR_packet(char *buffer, ssize_t &size, err_code_t code, const char* message)
{
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *err_code = (uint16_t *)&buffer[2];

    *opcode = ERR;
    *err_code = code;
    strcpy(&buffer[TFTP_HDR], message);

    size = TFTP_HDR + strlen(message) + 1;
}
