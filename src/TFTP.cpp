//====================================================================================================================
// Soubor:      TFTP.h
// Projekt:     VUT, FIT, ISA, TFTPv2 klient
// Datum:       24. 10. 2021
// Autor:       David Mihola
// Kontakt:     xmihol00@stud.fit.vutbr.cz
// Kompilovano: gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
// Popis:       Soubor definujici funkce pouzite pro tvorbu a analyzu TFTP paketu.
//====================================================================================================================

#include "TFTP.h"

void RQ_header(char *buffer, ssize_t &size, TFTP_options_t options)
{
    string value;
    uint16_t *opcode = (uint16_t *)buffer;
    size = options.file_URL.size() + 3; // 0 terminating byte a + 2 byty z opcode

    *opcode = options.opcode; // nastaveni opcode RRQ/WRQ
    strcpy(&buffer[2], options.file_URL.c_str()); // nsataveni URL prenaseneho souboru

    // nastaveni kodovani prenosu
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

    // blocksize, pokud ji uzivatel specifikoval
    if (options.block_size != TFTP_DATA_SIZE)
    {
        strcpy(&buffer[size], BLKSIZE);
        size += BLKSIZE_LEN;
        value = to_string(options.block_size);
        strcpy(&buffer[size], value.c_str());
        size += value.size() + 1;
    }

    // nastaveni tsize, pokud je prenos binarni, pri netascii to neni mozne
    if (options.transfer_size >= 0 && options.mode == BINARY)
    {
        strcpy(&buffer[size], TSIZE);
        size += TSIZE_LEN;
        value = to_string(options.transfer_size);
        strcpy(&buffer[size], value.c_str());
        size += value.size() + 1;
    }

    // nastaveni timeout, pokud jej uzivatel zadal
    if (options.timeout != 0)
    {
        strcpy(&buffer[size], TIMEOUT);
        size += TIMEOUT_LEN;
        value = to_string(options.timeout);
        strcpy(&buffer[size], value.c_str());
        size += value.size() + 1;
    }

    if (options.multicast && options.opcode == RRQ)
    {
        strcpy(&buffer[size], MULTICAST);
        size += MULTICAST_LEN;
        buffer[size++] = 0;
    }
}

void ACK_header(char *buffer, ssize_t &size, uint16_t ack_number)
{
    // ziskani pozic pro hodnoty
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];

    // nastaveni hodnot opcode a block number
    *opcode = ACK;
    *block_number = ack_number;
    buffer[4] = (char)0;
    size = 4;
}

negotiation_t parse_OACK(char *buffer, ssize_t size, bool blksize, bool timeout, bool tsize, bool multicast)//, sockaddr *address, socklen_t &addr_length)
{
    // nastaveni defaultnich hodnot
    negotiation_t negotiation;
    negotiation.block_size = -1;
    negotiation.transfer_size = -1;
    negotiation.multicast = false;
    negotiation.timeout = 0;
    ssize_t done = 2;
    string option;
    string value;

    while (done < size)
    {
        // ziskani jmena podminky prenosu
        option = string(&buffer[done]);
        done += option.size() + 1;
        // prevod na lowercase
        for_each(option.begin(), option.end(), [](char & c) 
        {
            c = tolower(c);
        });
        
        //ziskani hodnoty podminky prenosu
        value = string(&buffer[done]);
        done += value.size() + 1;

        // parsovani hodnoty na zaklade typu podminky
        if (option == BLKSIZE && blksize)
        {
            negotiation.block_size = stoi(value);
            if (negotiation.block_size < MIN_BLK_SIZE)
            {
                throw exception();    
            }
        }
        else if (option == TIMEOUT && timeout)
        {
            size_t res = stoul(value);
            if (res > UINT8_MAX)
            {
                // timeout out of range
                throw exception();    
            }
            negotiation.timeout = res;
        }
        else if (option == TSIZE && tsize)
        {
            negotiation.transfer_size = stol(value);
        }
        else if (option == MULTICAST && multicast)
        {
            size_t pos = value.find(',');
            if (pos != string::npos)
            {
                value[pos] = '\0';
                if (!inet_pton(AF_INET, value.c_str(), &negotiation.address.IPv4.sin_addr))
                {
                    if (!inet_pton(AF_INET6, value.c_str(), &negotiation.address.IPv6.sin6_addr))
                    {
                        throw exception();
                    }
                    negotiation.address.IPv6.sin6_family = AF_INET6;
                }
                else
                {
                    negotiation.address.IPv4.sin_family = AF_INET;
                }
            }
            else
            {
                if (value.size() == 0)
                {
                    done++;
                    negotiation.multicast = false;
                    continue;
                }
                // adresa nelze ziskat
                throw exception();
            }

            value = value.substr(pos + 1);
            pos = value.find(',');
            if (pos != string::npos)
            {
                value[pos] = '\0';
                unsigned long port = stoul(value);
                if (port > UINT16_MAX)
                {
                    // cislo portu out of range
                    throw exception();
                }
                negotiation.address.IPv6.sin6_port = htons(port);
            }
            else
            {
                // port nelze ziskat
                throw exception();
            }

            if (value[pos + 1] == '1')
            {
                // tento klient je master klient
                negotiation.multicast = true;
            }
            else
            {
                // nejedna se o master klienta, neni podporovano
                throw exception();
            }
        }
        else
        {
            // neznama podminka, client nespecifikoval
            throw exception();
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
            return "(8) Option negotiation failed.";
    
        default:
            return "Uknonw error code.";
    }
}

void ERR_packet(char *buffer, ssize_t &size, err_code_t code, const char* message)
{
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *err_code = (uint16_t *)&buffer[2];

    // nastaveni opcode a error code
    *opcode = ERR;
    *err_code = code;
    // vlozeni chybove zpravy
    strcpy(&buffer[TFTP_HDR], message);
    
    size = TFTP_HDR + strlen(message) + 1;
}
