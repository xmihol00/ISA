//====================================================================================================================
// Soubor:      in_out.h
// Projekt:     VUT, FIT, ISA, TFTPv2 klient
// Datum:       30. 10. 2021
// Autor:       David Mihola
// Kontakt:     xmihol00@stud.fit.vutbr.cz
// Kompilovano: gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
// Popis:       Soubor deklarujici funkce, struktury a jine hodnoty pouzite pro ziskavani dat od uzivatele 
//              a vypis vysledku prenosu souboru.
//====================================================================================================================

#pragma once

#include <chrono>
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
using namespace std::chrono;
using namespace std::chrono::_V2;

#define DEFAULT_PORT 69U
#define DEFAULT_TIMEOUT 1U
#define CR 13
#define LF 10
#define MAX_URL_LEN 512
#define MIN_BLK_SIZE 4

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
    transfer_mode_t     transfer_mode;  // typ prenostu
    string              file_URL;       // cesta a jmeno prenaseneho souboru
    uint8_t             timeout;        // znoleny timeout, defaultne 0
    int                 block_size;     // zvolena velikost bloku, defaultne 512 B
    data_mode_t         data_mode;      // ty zakodovani dat pro prenos
    address_type_t      address_type;   // typ IP adresy
    union
    {
        in_addr ipv4;
        in6_addr ipv6;
    }                   address;        // IP adresa
    string              address_str;    // textova reprezentace IP adresy
    in_port_t           port;           // cislo portu
    bool                multicast;      // true pro zvoleny multicast, defaultne false
} arguments_t;

typedef struct 
{
    bool             success;           // true pri uspechu prenenosu, jinak false
    string           file;              // jmeno souboru
    transfer_mode_t  mode;              // typ prenosu cteni/zapis
    int              blksize;           // velikost prenaseneho bloku
    unsigned         datagram_count;    // pocet prenesenych datagramu
    int64_t          data_size;         // objem prenesenych dat v B
    unsigned         lost_count;        // pocet pravdepodobne ztracenych datagramu
    int64_t          lost_size;         // pravdepodobne ztracena data v prubehu prenosu v B 
} transfer_summary_t;

/**
 * @brief Vytiskne zpravu s napovedou na stdin.
 */
void help_msg();

/**
 * @brief Parsuje jeden radek uzivatelskeho vstupu ze stdin.
 * @param line radek vstupu.
 * @param arguments ziskane argumety ze vstupniho radku, pri navratove hodnote true neni definovano. 
 *                  Jinak na staveno na ziskane hodnoty nebo defaultni hodnoty.
 * @return 1 argumetns accepted, 0 argumetns not accepted, -1 terminate application
 */
int parse_line(const string &line, arguments_t &arguments);

/**
 * @brief Ziska jmeno souboru z cesty.
 * @param file_URL cesta k souboru.
 * @param return jmeno souboru.
 */
string get_file_name(const string &file_URL);

/**
 * @brief ziska mnozstvi vlneho mista na disku.
 * @return volne misto na disku v B.
 */
long available_space();

/**
 * @brief Precte znaky z ze souboru a konvertuje je z radku zakoncenych LF na CRLF a CR na CR\0
 * @param file cteny soubor.
 * @param block_size maximalni prectena velikost.
 * @param buffer buffer do ktereho jsou prectene znaky ulozeny. 
 */
ssize_t fread_to_netascii(FILE *file, ssize_t block_size, char *buffer);

/**
 * @brief Zapise znaky z bufferu do souboru a konvertuje je z netascii na radky zakoncene LF.
 * @param file vystupni soubor.
 * @param size velikost dat v bufferu.
 * @param block_size maximalni zapsana velikost.
 * @param buffer obsahuje data.
 * @return flase pri uspechu, jinak true.
 */
bool fwrite_from_netascii(FILE *file, ssize_t size, ssize_t block_size, char *buffer);

/**
 * @brief Vytiskne shrnuti prenosu souboru.
 * @param summary struktura obsahujici informace o probehlem prenosu.
 * @param start cas zacatku prenosu.
 * @param end cas konce prenosu.
 */
void print_summary(const transfer_summary_t &summary, system_clock::time_point start, system_clock::time_point end);
