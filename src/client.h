//====================================================================================================================
// Soubor:      client.h
// Projekt:     VUT, FIT, ISA, TFTPv2 klient
// Datum:       24. 10. 2021
// Autor:       David Mihola
// Kontakt:     xmihol00@stud.fit.vutbr.cz
// Kompilovano: gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
// Popis:       Soubor deklarujci funkce a struktury pro cteni a psani dat na/ze serveru.
//====================================================================================================================

#pragma once

using namespace std;

#include "in_out.h"
#include "TFTP.h"

#include <chrono>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>

#define ACK_BUFFER_SIZE 1024U
#define MAX_IPv4_HDR 60
#define UDP_HDR 8
#define IPv6_HDR 40
#define PADDING 2

typedef struct
{
    const string    &file_URL;      // cesta a jmeno souboru
    data_mode_t     mode;           // typ prenosu
    int32_t         block_size;     // velikost jednoho bloku prenesenych dat v B
    bool            multicast;      // true, pokud multicast, jinak flase
    uint8_t         timeout;        // doba timeoutu v s
} transfer_data_t;

/**
 * @brief Provede prenos souboru.
 * @param argumets argumety prenosu.
 */
void transfer(const arguments_t &arguments);

/**
 * @brief Provede prenos souboru ze serveru na klienta.
 * @param socket_fd file descriptor pouziteho socketu.
 * @param address adresa serveru.
 * @param addr_length delka adresy.
 * @param data data o provadenem prenosu.
 * @return shrnuti prenosu.
 */
transfer_summary_t read(int socket_fd, struct sockaddr *address, socklen_t addr_length, transfer_data_t data);

/**
 * @brief Provede prenos souboru z klinta na server.
 * @param socket_fd file descriptor socketu.
 * @param address adresa serveru.
 * @param addr_lenght delka adresy.
 * @param data data o provadenem prenosu.
 * @return shrnuti prenosu.
 */
transfer_summary_t write(int socket_fd, struct sockaddr *address, socklen_t addr_length, transfer_data_t data);

/**
 * @brief Ziska MTU pouziteho zarizeni pri odesilani na danou adresu.
 * @param address adresa prijemce.
 * @param lenght delka adresy.
 * @return velikost MTU rozhrani.
 */
int32_t get_MTU_of_used_if(sockaddr_in6 address, socklen_t lenght);

/**
 * @brief ziska nejmensi MTU z rozhrani na systemu.
 * @param destination adresa destinace.
 * @return nejmensi velikost MTU na systemu.
 */
int32_t get_min_MTU(sa_family_t address_family);

/**
 * @brief Nastavi timeout socketu.
 * @param socket_fd file descriptor socketu.
 * @param timeout nastavovany timeout.
 */
void set_timeout(int socket_fd, uint8_t timeout);

/**
 * @brief Nastavi podminky prenosu odsouhlasene serverem.
 * @param socket_fd file descriptor socketu pouzivaneho pro kominakci se serverem.
 * @param address adresa serveru.
 * @param addr_lenght delka adresy.
 * @param buffer buffer obsahujcic OACK zpravu od serveru.
 * @param size velikost zpravy v bufferu.
 * @param data vepise ziskane podminky odsouhlasene serverem.
 */
bool set_negotioation(int &socket_fd, char *buffer, ssize_t &size, transfer_data_t &data);
