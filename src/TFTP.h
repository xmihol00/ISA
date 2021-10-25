//====================================================================================================================
// Soubor:      TFTP.h
// Projekt:     VUT, FIT, ISA, TFTPv2 klient
// Datum:       24. 10. 2021
// Autor:       David Mihola
// Kontakt:     xmihol00@stud.fit.vutbr.cz
// Kompilovano: gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
// Popis:       Soubor deklarujici funkce, struktury a jina data pouzite pro tvorbu a analyzu TFTP paketu.
//====================================================================================================================

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

#include "in_out.h"

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
#define MULTICAST "multicast"
#define MULTICAST_LEN 10

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
    const string    &file_URL;      // cesta a jmeno souboru
    opcode_t        opcode;         // opcode TFTP packetu
    data_mode_t     mode;           // typ zakodovani prenasenych dat
    int             block_size;     // velikost prenaseneho bloku dat
    long            transfer_size;  // velikost prenaseneho souboru, defaultne 0
    uint8_t         timeout;        // doba timeout v s, defaultne 0
    bool            multicast;      // signalizuje pozuiti muticastu, defaultne false
} TFTP_options_t;

typedef struct
{
    int         block_size;         // velikost prenaseneho bloku, defaultne -1
    long        transfer_size;      // velikost prenaseneho souboru, defualtne -1
    uint8_t     timeout;            // timeout v s, defaultne 0 
    bool        multicast;          // true pokud server odpovedel s multicast, jinak false
    union
    {
        sockaddr_in IPv4;
        sockaddr_in6 IPv6;
    } address;                      // multicast IP adresa
} negotiation_t;

/**
 * @brief Vytvori RRQ nebo WRQ header potencionalne s rozsirujicimi moznostmi prenosu.
 * @param buffer buffer, do ktereho budou vepsana data.
 * @param size velikost zapsanych dat v bytech.
 * @param options podminky prenosu.
 */
void RQ_header(char *buffer, ssize_t &size, TFTP_options_t options);

/**
 * @brief Vytvori ACK header.
 * @param bufffer buffer, do ktereho budou vepsana data.
 * @param size velikost zapsanych dat.
 * @param asc_number cislo paketu, naktery je vytvaren ACK header.
 */
void ACK_header(char *buffer, ssize_t &size, uint16_t ack_number);

/**
 * @brief Vytvori ERR header.
 * @param bufffer buffer, do ktereho budou vepsana data.
 * @param size velikost zapsanych dat.
 * @param code kod chyby.
 * @param message chybova zprava.
 */
void ERR_packet(char *buffer, ssize_t &size, err_code_t code, const char* message);

/**
 * @brief Prelozi kod chyby na jeji textovy popis.
 * @param err_code kod chyby.
 * @return textova reprezentace chyby.
 */
string err_code_value(uint16_t err_code);

/**
 * @brief Ziska s OACK header hodnoty podminek prijatych serverem
 * @param buffer OACK packet.
 * @param size velikost dat packetu.
 * @param blksize true, pokud byl zaslan dotaz na velikost bloku, jinak false.
 * @param timeout true, pokud byl zaslan dotaz na timeout, jinak false.
 * @param tsize true, pokud byl zaslan dotaz na velikost prenaseneho souboru, jinak false.
 * @return ziskane nebo defaultni hodnoty podminek.
 */
negotiation_t parse_OACK(char *buffer, ssize_t size, bool blksize, bool timeout, bool tsize, bool multicast); //, sockaddr *address, socklen_t &addr_length);
