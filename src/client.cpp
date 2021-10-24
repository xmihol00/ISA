//====================================================================================================================
// Soubor:      client.cpp
// Projekt:     VUT, FIT, ISA, TFTPv2 klient
// Datum:       24. 10. 2021
// Autor:       David Mihola
// Kontakt:     xmihol00@stud.fit.vutbr.cz
// Kompilovano: gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
// Popis:       Soubor implementujici funkce pro cteni a zapis souboru na/ze serveru.
//====================================================================================================================

#include "client.h"

void transfer(const arguments_t &arguments)
{
    // ziskani casu zacatku prenosu
    system_clock::time_point start = high_resolution_clock::now();
    struct sockaddr_in server_v4;
    struct sockaddr_in6 server_v6;
    struct sockaddr *destination = nullptr;
    socklen_t destination_len = 0;
    int socket_fd = 0;
    int32_t mtu = arguments.block_size;

    // otevreni socketu na zaklade typu adresy
    if (arguments.address_type == IPv4)
    {
        destination_len = sizeof(server_v4);
        memset(&server_v4, 0, destination_len);
        server_v4.sin_addr = arguments.address.ipv4;
        server_v4.sin_family = AF_INET;
        server_v4.sin_port = arguments.port;
        destination = (struct sockaddr *)&server_v4;
        socket_fd = socket(AF_INET , SOCK_DGRAM , 0);
    }
    else
    {
        destination_len = sizeof(server_v6);
        memset(&server_v6, 0, destination_len);
        server_v6.sin6_addr = arguments.address.ipv6;
        server_v6.sin6_family = AF_INET6;
        server_v6.sin6_port = arguments.port;
        destination = (struct sockaddr *)&server_v6;
        socket_fd = socket(AF_INET6 , SOCK_DGRAM , 0);
    }
    if (socket_fd == -1) // otevreni socketu selhalo
    {
        cerr << "Error: Socket could no be created." << endl;
        return;
    }

    if (arguments.block_size != TFTP_DATA_SIZE)
    {
        // ziskani minialniho MTU pro prenos dat v systemu
        if ((mtu = get_min_MTU(destination->sa_family)) == INT32_MAX) 
        {
            cerr << "Warning: MTU could not be obtained, default block size of 512 is used instead." << endl;
        }
        else
        {
            // snizeni MTU o velikosti hlavicek nizzsich vrstev
            if (destination->sa_family == AF_INET) // IPv4
            {
                mtu -= MAX_IPv4_HDR + UDP_HDR + TFTP_HDR;
                if (mtu >= arguments.block_size)
                {
                    mtu = arguments.block_size > 0 ? arguments.block_size : mtu;
                }
                else
                {
                    cerr << "Warning: Specified block size exceeds minimum MTU of size " << mtu << ", which is used instead." << endl;
                }
            }
            else // IPv6
            {
                mtu -= IPv6_HDR + UDP_HDR + TFTP_HDR;
                if (mtu >= arguments.block_size)
                {
                    mtu = arguments.block_size > 0 ? arguments.block_size : mtu;
                }
                else
                {
                    cerr << "Warning: Specified block size exceeds minimum MTU of size " << mtu << ", which is used instead." << endl;
                }
            }
        }
    }

    // nastaveni timeout socketu
    set_timeout(socket_fd, arguments.timeout);

    transfer_summary_t summary;
    // cteni nebo zapis souboru
    if (arguments.transfer_mode == READ)
    {
        summary = read(socket_fd, destination, destination_len, {arguments.file_URL, arguments.data_mode, mtu, arguments.multicast, arguments.timeout});
    }
    else
    {
        summary = write(socket_fd, destination, destination_len, {arguments.file_URL, arguments.data_mode, mtu, arguments.multicast, arguments.timeout});
    }

    close(socket_fd);
    
    // cas konce prenosu
    system_clock::time_point end = high_resolution_clock::now();
    print_summary(summary, start, end);
}

transfer_summary_t read(int socket_fd, struct sockaddr *address, socklen_t addr_length, transfer_data_t data)
{
    uint8_t retries = 0; // pocet pokusu znovu odeslani ztraceneho datagramu
    in_port_t TID = 0; // TID serveru
    // inicializace podminek prenosu
    TFTP_options_t options = { .file_URL = data.file_URL, .opcode = RRQ, .mode = data.mode, .block_size = data.block_size, 
                               .transfer_size = 0, .timeout = data.timeout, .multicast = data.multicast };
    // inicializace prehledu prenosu, bude vyplnena v prubehu prenosu
    transfer_summary_t summary = { .success = false, .file = get_file_name(data.file_URL), .mode = READ, .blksize = TFTP_DATA_SIZE, 
                                   .datagram_count = 0, .data_size = 0, .lost_count = 0, .lost_size = 0 };
    

    // buffer pro prijem dat
    char *buffer = new char[(data.block_size > TFTP_DATA_SIZE ? data.block_size + TFTP_HDR : TFTP_DATAGRAM_SIZE) + PADDING]();
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];
    uint16_t ack_number;

    // buffer pro zasilani ACK
    char ack_buff[ACK_BUFFER_SIZE] = {0, };

    ssize_t size = 0;
    struct sockaddr_in6 recieved_address;
    memcpy(&recieved_address, address, addr_length);
    struct sockaddr *ptr_recieved_address = (struct sockaddr *) &recieved_address;
    socklen_t recv_lenght = sizeof(recieved_address);
    
    FILE *file = fopen(summary.file.c_str(), "w");
    if (file == nullptr)
    {
        cerr << "Error: Could not open file '" << summary.file << "' for writing." << endl;
        goto data_cleanup;
    }
    cout << "Reading file '" << summary.file << "' ..." << endl;

    do
    {
        // preventivni nastaveni ztraty datagramu
        summary.lost_count++;

        // nastaveni defaultniho TID serveru (69 nebo specifikovano uzivatelelem)
        ((sockaddr_in *)address)->sin_port = recieved_address.sin6_port; 
        
        if (retries++ > MAX_RETRIES)
        {
            cerr << "Error: Transfer of data failed. Server timed out." << endl;
            size = data.block_size;
            goto cleanup;
        }

        RQ_header(ack_buff, size, options);
        if (sendto(socket_fd, ack_buff, size, 0, address, addr_length) != size)
        {
            cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
            ERR_packet(buffer, size, NOT_DEFINED, "Data could not be send fully.");
            sendto(socket_fd, buffer, size, 0, address, addr_length);
            goto cleanup;
        }
    // snaha o navazani komuniakce se serverem    
    }
    while ((size = recvfrom(socket_fd, buffer, TFTP_DATAGRAM_SIZE, 0, ptr_recieved_address, &recv_lenght)) == -1 || // recv selhala
           (*opcode == DATA && ntohs(*block_number) != 1)); // prichozi datagram je nespravny
    retries = 0;
    // ziskani TID serveru
    ((sockaddr_in *)address)->sin_port = TID = recieved_address.sin6_port;
    ack_number = *block_number;
    summary.lost_count--; // jeden datagram urcite ztracen nebyl
    summary.datagram_count++;

    // server rozumi rozireni o podminkach prenosu
    if (*opcode == OACK)
    {
        // ziskani podminek prenosu odsouhlasenych serverem
        if (set_negotioation(socket_fd, address, addr_length, buffer, size, data))
        {
            size = data.block_size;
            summary.datagram_count = 0;
            goto cleanup;
        }

        // resetovani odrzenych dat
        summary.datagram_count = 0;
        summary.lost_count = 0;
        summary.blksize = data.block_size;

        while(true) // dokud neprisel datagram se spravnym TID
        {
            do
            {
                summary.lost_count++;

                if (retries++ > MAX_RETRIES)
                {
                    cerr << "Error: Transfer of data failed. Server timed out." << endl;
                    size = data.block_size;
                    goto cleanup;
                }

                ACK_header(buffer, size, 0);
                // odeslani ACK na OACK
                if (sendto(socket_fd, buffer, size, 0, address, addr_length) != size)
                {
                    cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
                    ERR_packet(buffer, size, NOT_DEFINED, "Data could not be send fully.");
                    sendto(socket_fd, buffer, size, 0, address, addr_length);
                    goto cleanup;
                }
            
            // prijmuti 1. datoveho bloku
            }
            while ((size = recvfrom(socket_fd, buffer, data.block_size + TFTP_HDR, 0, ptr_recieved_address, &recv_lenght)) == -1 || // recv neselhal
                   (recieved_address.sin6_port == TID && ntohs(*block_number) != 1 && *opcode == DATA)); // prichozi datagram je nespravny

            if (recieved_address.sin6_port != TID) // spatne TID
            {
                cerr << "Warning: Recieved TID does not match the estabhlished one, server informed and transfer continues." << endl;
                ERR_packet(ack_buff, size, UNKNOWN_ID, "Incorrect TID.");
                sendto(socket_fd, ack_buff, size, 0, address, addr_length);
                retries--;
                // prenos pokracuje
                continue;
            }

            summary.lost_count--; // jeden datagram nebyl ztrace
            summary.datagram_count++; // jeden datagram byl dorucen
            retries = 0;
            break;
        }
        ack_number = *block_number; // nastaveni aktuakniho cisla ACK
    }
    // server nerozumi podminkam prenosu
    else if (*opcode == DATA)
    {
        cerr << "Warning: Server does not support Option Negotiation." << endl;
        data.block_size = TFTP_DATA_SIZE;
    }
    else if (*opcode == ERR)
    {
        buffer[size] = 0;
        cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[TFTP_HDR] << endl;
        goto cleanup;
    }
    else
    {
        cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
        ERR_packet(buffer, size, NOT_DEFINED, "Unexpected packet recieved.");
        sendto(socket_fd, buffer, size, 0, address, addr_length);
        goto cleanup;
    }

    size -= TFTP_HDR;
    if (data.mode == BINARY) // binarni prenos
    {
        fwrite(&buffer[TFTP_HDR], 1, size, file);
    }
    else // prenos pomoci netascii
    {
        // parsovani netascii 
        if (fwrite_from_netascii(file, size, &buffer[TFTP_HDR]))
        {
            cerr << "Error: Malformed packet recieved." << endl;
            ERR_packet(ack_buff, size, NOT_DEFINED, "Malformed packet.");
            sendto(socket_fd, ack_buff, size, 0, address, addr_length);
            goto cleanup;
        }
    }

    // dokud neni prijat posledni blok souboru
    while (size == data.block_size)
    {
        while(true) // dokud prichazi datagramy se spatnym TID
        {
            do
            {
                summary.lost_count++;

                if (retries++ > MAX_RETRIES)
                {
                    cerr << "Error: Transfer of data failed. Server timed out." << endl;
                    size = data.block_size;
                    goto cleanup;
                }
                // ACK posledniho datoveho packetu 
                ACK_header(ack_buff, size, ack_number);
                if (sendto(socket_fd, ack_buff, size, 0, address, addr_length) != size)
                {
                    cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
                    ERR_packet(buffer, size, NOT_DEFINED, "Data could not be send fully.");
                    sendto(socket_fd, buffer, size, 0, address, addr_length);
                    goto cleanup;
                }
            } 
            while ((size = recvfrom(socket_fd, buffer, data.block_size + TFTP_HDR, 0, ptr_recieved_address, &recv_lenght)) == -1 || // recv neselhal
                   (recieved_address.sin6_port == TID && ntohs(*block_number) != ntohs(ack_number) + 1 && *opcode == DATA)); // ziskany datagram je nespravny
            
            if (recieved_address.sin6_port != TID) // spatne TID
            {
                cerr << "Warning: Recieved TID does not match the estabhlished one, server informed and transfer continues." << endl;
                ERR_packet(ack_buff, size, UNKNOWN_ID, "Incorrect TID.");
                sendto(socket_fd, ack_buff, size, 0, address, addr_length);
                // prenos pokracuje
                continue;
            }

            ack_number = *block_number; // zikani aktulaniho cisla bloku, ktery bude nasledovne nutne odsouhlasit
            summary.lost_count--;
            summary.datagram_count++;
            break;
        }

        if (*opcode != DATA) // kontrola obdrzenych dat
        {
            if (*opcode == ERR)
            {
                buffer[size] = 0;
                cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[TFTP_HDR] << endl;
                goto cleanup;
            }
            else // data s neznamym opcode
            {
                cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
                ERR_packet(buffer, size, NOT_DEFINED, "Unexpected packet recieved.");
                sendto(socket_fd, buffer, size, 0, address, addr_length);
                goto cleanup;
            }
        }   

        retries = 0;
        size -= TFTP_HDR;
        if (data.mode == BINARY) // zapis binarniho souboru
        {
            fwrite(&buffer[TFTP_HDR], 1, size, file);
        }
        else
        {
            // konverze a zapis netascii souboru
            if (fwrite_from_netascii(file, size, &buffer[TFTP_HDR]))
            {
                cerr << "Error: Malformed packet recieved." << endl;
                ERR_packet(ack_buff, size, NOT_DEFINED, "Malformed packet.");
                sendto(socket_fd, ack_buff, size, 0, address, addr_length);
                goto cleanup;
            }
        }
    }
    // cteni probehlo uspesne
    summary.success = true;

    // zaslani posledniho ACK
    ACK_header(buffer, summary.data_size, *block_number);
    sendto(socket_fd, buffer, summary.data_size, 0, address, addr_length);

// uvolneni zdroju
cleanup:
    fclose(file);
data_cleanup:
    delete[] buffer;

    // vypocet obdrzenych dat a ztrat
    summary.data_size = size + summary.datagram_count * data.block_size - data.block_size;
    summary.lost_size = summary.lost_count * data.block_size;

    return summary;
};

transfer_summary_t write(int socket_fd, struct sockaddr *address, socklen_t addr_length, transfer_data_t data)
{
    uint8_t retries = 0;  // pocet pokusu o znovuodeslani ztraceneho datagramu
    in_port_t TID = 0;    // TID serveru
    // inicializace podminek prenosu
    TFTP_options_t options = { .file_URL = data.file_URL, .opcode = WRQ, .mode = data.mode, .block_size = data.block_size, 
                               .transfer_size = 0, .timeout = data.timeout, .multicast = data.multicast };
    // inizializace prehledu zapsani souboru na server, v prubehu bude upravovana
    transfer_summary_t summary = { .success = false, .file = get_file_name(data.file_URL), .mode = WRITE, .blksize = TFTP_DATA_SIZE, 
                                   .datagram_count = 0, .data_size = 0, .lost_count = 0, .lost_size = 0 };
    
    // buffer pro odesilana data
    char *buffer = new char[(data.block_size > TFTP_DATA_SIZE ? data.block_size + TFTP_HDR : TFTP_DATAGRAM_SIZE) + PADDING]();
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];
    
    // buffer pro prijem ACK
    char ack_buff[ACK_BUFFER_SIZE] = {0, };
    uint16_t *ack_opcode = (uint16_t *)ack_buff;
    uint16_t *ack_block_number = (uint16_t *)&ack_buff[2];

    // adresa pro prijem datagramu
    ssize_t size = 0;
    ssize_t err_size = ACK_BUFFER_SIZE;
    struct sockaddr_in6 recieved_address;
    memcpy(&recieved_address, address, addr_length);
    struct sockaddr *ptr_recieved_address = (struct sockaddr *) &recieved_address;
    socklen_t recv_lenght = sizeof(recieved_address);

    FILE *file = fopen(data.file_URL.c_str(), "r");
    if (file == nullptr)
    {
        cerr << "Error: Could not open file '" << summary.file << "' for reading." << endl;
        goto data_cleanup;
    }

    // ziskani velikosti odesilaneho souboru
    if (fseek(file, 0, SEEK_END) == -1 || (options.transfer_size = ftell(file)) == -1 || fseek(file, 0, SEEK_SET) == -1)
    {
        rewind(file);
        cerr << "Warning: Could not obtain size of file '" << summary.file << "', 'tsize' option will not be used." << endl;
        options.transfer_size = -1;
    }

    // zacatek zapisu dat na server
    cout << "Writing file '" << summary.file << "' ..." << endl;
    do // kontaktovani serveru
    {
        // nastaveni defaultniho TID serveru (69 nebo specifikovano uzivatelelem)
        ((sockaddr_in *)address)->sin_port = recieved_address.sin6_port;
        
        if (retries++ > MAX_RETRIES)
        {
            cerr << "Error: Transfer of data failed. Server timed out." << endl;
            goto cleanup;
        }

        RQ_header(ack_buff, size, options);
        if (sendto(socket_fd, ack_buff, size, 0, address, addr_length) != size)
        {
            cerr << strerror(errno) << endl;
            cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
            ERR_packet(buffer, size, NOT_DEFINED, "Data could not be send fully.");
            sendto(socket_fd, buffer, size, 0, address, addr_length);
            summary.datagram_count++;
            summary.data_size += size;
            goto cleanup;
        }
    } 
    while ((size = recvfrom(socket_fd, ack_buff, ACK_BUFFER_SIZE, 0, ptr_recieved_address, &recv_lenght)) == -1 || // recv selhalo
           (*ack_opcode == ACK && *ack_block_number != 0)); // nespravny ACK datagram
    
    ((sockaddr_in *)address)->sin_port = TID = recieved_address.sin6_port; // nastaveni TID serveru
    retries = 0;

    if (*ack_opcode == OACK) // server podporuje option negotiation
    {
        if (set_negotioation(socket_fd, address, addr_length, ack_buff, size, data))
        {
            summary.datagram_count++;
            summary.data_size += size;
            goto cleanup;
        }
        summary.blksize = data.block_size;
    }
    else if (*ack_opcode == ACK) // server nepodporuje option negotiation
    {
        cerr << "Warning: Server does not support Option Negotiation." << endl;
        data.block_size = TFTP_DATA_SIZE;
    }
    else if (*ack_opcode == ERR) // znama chyba
    {
        buffer[size] = 0;
        cerr << "Error: " << err_code_value(*ack_block_number) << " Message: " << &ack_buff[TFTP_HDR] << endl;
        goto cleanup;
    }
    else // jina chyba
    {
        cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
        ERR_packet(buffer, size, NOT_DEFINED, "Unexpected packet recieved.");
        sendto(socket_fd, buffer, size, 0, address, addr_length);
        summary.datagram_count++;
        summary.data_size += size;
        goto cleanup;
    }

    for (uint16_t i = 1; ; i++) 
    {
        *opcode = DATA; // opcode datagramu
        *block_number = htons(i); // cislo bloku ve spravnem endianu
        if (data.mode == BINARY) // binarni prenos
        {
            size = fread(&buffer[TFTP_HDR], 1, data.block_size, file);
        }
        else // netascii prenos
        {
            size = fread_to_netascii(file, data.block_size, &buffer[TFTP_HDR]);
        }

        while(true) // dokud prichazi datagramy se spatnym TID
        {
            do
            {
                // vypocet odeslanych a ztracenych dat
                summary.datagram_count++;
                summary.data_size += size;
                summary.lost_count++;
                summary.lost_size += size;

                if (retries++ > MAX_RETRIES)
                {
                    cerr << "Error: Transfer of data failed. Server timed out." << endl;
                    goto cleanup;
                }

                if (sendto(socket_fd, buffer, size + TFTP_HDR, 0, address, addr_length) != size + TFTP_HDR)
                {
                    cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
                    ERR_packet(buffer, size, NOT_DEFINED, "Data could not be send fully.");
                    sendto(socket_fd, buffer, size, 0, address, addr_length);
                    summary.datagram_count++;
                    summary.data_size += size;
                    goto cleanup;
                }
            }
            while (recvfrom(socket_fd, ack_buff, ACK_BUFFER_SIZE, 0, ptr_recieved_address, &recv_lenght) == -1 ||  // recv je neuspesna
                   (recieved_address.sin6_port == TID && *ack_block_number != *block_number && *ack_opcode == ACK)); // prijaty datagram neni spravny
            
            if (recieved_address.sin6_port != TID)
            {
                cerr << "Warning: Recieved TID does not match the estabhlished one, server informed and transfer continues." << endl;
                ERR_packet(ack_buff, err_size, UNKNOWN_ID, "Incorrect TID.");
                sendto(socket_fd, ack_buff, err_size, 0, address, addr_length);
                retries--;
                continue;
            }

            retries = 0;
            // jeden datagram se jiste neztratil
            summary.lost_count--; 
            summary.lost_size -= size;
            break;
        } 
        
        if (*ack_opcode != ACK) // chyby
        {
            if (*ack_opcode == ERR) // znama chyba
            {
                buffer[size] = 0;
                cerr << "Error: " << err_code_value(*ack_block_number) << " Message: " << &ack_buff[TFTP_HDR] << endl;
                goto cleanup;
            }
            else // nerozpoznatelny nebo spatny opcode
            {
                cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
                ERR_packet(buffer, size, NOT_DEFINED, "Unexpected packet recieved.");
                sendto(socket_fd, buffer, size, 0, address, addr_length);
                summary.datagram_count++;
                summary.data_size += size;
                goto cleanup;
            }
        }

        if (size < data.block_size) // konec prenosu, pokud je odeslana veliksot mensi, nez velikost bloku
        {
            break;
        }
    }

    // prenos probehl uspesne
    summary.success = true;

// uvolneni zdroju
cleanup:
    fclose(file);
data_cleanup:
    delete[] buffer;

    return summary;
}

void set_timeout(int socket_fd, uint8_t timeout)
{
    struct timeval time_out;
    time_out.tv_sec = timeout == 0 ? DEFAULT_TIMEOUT : timeout; // minimalni timeout je 1
    time_out.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out)) == -1)
    {
        cerr << "Warning: Timeout could not be set, program might halt. In this case use Ctrl+C to terminate." << endl;
    }
}

// funkce nepouzita
int32_t get_MTU_of_used_if(sockaddr_in6 address, socklen_t lenght)
{
    sockaddr *destination = (sockaddr *)&address;
    struct ifaddrs *current = nullptr, *head = nullptr;
    struct ifreq request;
    int socket_fd;
    int32_t mtu = INT32_MAX;

    if ((socket_fd = socket(destination->sa_family, SOCK_DGRAM, 0)) == -1)
    {
        goto failed;
    }

    if (connect(socket_fd, destination, lenght) == -1)
    {
        goto failed;
    }

    // ziskani IP adresy socketu
    if (getsockname(socket_fd, destination, &lenght) == -1)
    {
        goto failed;
    }

    // ziskani vsech interfacu
    if (getifaddrs(&head) == -1)
    {
        goto failed;
    }

    for (current = head; current != nullptr; current = current->ifa_next)
    {
        if (current->ifa_name != nullptr)
        {
            strncpy(request.ifr_name, current->ifa_name, sizeof(request.ifr_name));
            // ziskani IP adresy rozhrani 
            if (ioctl(socket_fd, SIOCGIFADDR, &request) == -1)
            {
                continue;
            }

            if (request.ifr_addr.sa_family == destination->sa_family)
            {
                // adresy se schoduji
                if ((destination->sa_family == AF_INET && 
                    ((struct sockaddr_in *)&request.ifr_addr)->sin_addr.s_addr == ((struct sockaddr_in *)destination)->sin_addr.s_addr) ||
                    (destination->sa_family == AF_INET6 && 
                    memcmp(&((struct sockaddr_in6 *)&request.ifr_addr)->sin6_addr, &((struct sockaddr_in6 *)destination)->sin6_addr, sizeof(struct in6_addr))))
                {
                    // ziskani MTU zarizeni
                    if (ioctl(socket_fd, SIOCGIFMTU, &request) == -1)
                    {
                        continue;
                    }
                    
                    mtu = request.ifr_mtu;
                    break;
                }
            }
        }
    }

// uvolneni zdroju
failed:
    close(socket_fd);
    freeifaddrs(head);
    return mtu;
}

int32_t get_min_MTU(sa_family_t address_family)
{
    int socket_fd;
    int32_t mtu = INT32_MAX;
    struct ifaddrs *current = nullptr, *head = nullptr;
    struct ifreq request;
    
    if ((socket_fd = socket(address_family, SOCK_DGRAM, 0)) == -1)
    {
        goto failed;
    }

    if (getifaddrs(&head) == -1)
    {
        goto failed;
    }

    for (current = head; current != nullptr; current = current->ifa_next)
    {
        if (current->ifa_name != nullptr)
        {
            strncpy(request.ifr_name, current->ifa_name, sizeof(request.ifr_name));
            // ziskani MTU socketu
            if (ioctl(socket_fd, SIOCGIFMTU, &request) == -1)
            {
                continue;
            }
            
            // nastaveni nejmensiho MTU
            if (mtu > request.ifr_mtu)
            {
                mtu = request.ifr_mtu;
            }
        }
    }

// uvolneni zdroju
failed:
    close(socket_fd);
    freeifaddrs(head);
    return mtu;
}

bool set_negotioation(int socket_fd, struct sockaddr *address, socklen_t addr_length, char *buffer, ssize_t &size, transfer_data_t &data)
{
    negotiation_t negotiation;
    
    try
    {
        // parsovani odpovedi serveru
        negotiation = parse_OACK(buffer, size, data.block_size != TFTP_DATA_SIZE, data.timeout != 0, data.mode == BINARY);
    }
    catch(const std::exception& e)
    {
        cerr << "Error: Could not parse Option Negotiation datagram, unknown or unspecified option recieved." << endl;
        ERR_packet(buffer, size, BAD_OACK, "Could no parse OACK datagram.");
        sendto(socket_fd, buffer, size, 0, address, addr_length);
        return true;
    }

    if (data.block_size != TFTP_DATA_SIZE)
    {
        if (negotiation.block_size == -1) // server neodpovedel na velikost bloku
        {
            cerr << "Warning: Server did not recognize block size option. Default size of 512 B is used instead" << endl;
            data.block_size = TFTP_DATA_SIZE;
        }
        else if (negotiation.block_size <= data.block_size) // server odpovedel spravne
        {
            data.block_size = negotiation.block_size;
            if (data.block_size < MIN_BLK_SIZE)
            {
                cerr << "Error: Server specified to small block size." << endl;
                ERR_packet(buffer, size, BAD_OACK, "Block size too small.");
                sendto(socket_fd, buffer, size, 0, address, addr_length);
                return true;
            }
        }
        else // server odpovedel chybne
        {
            cerr << "Error: Server specified block size larger than offered." << endl;
            ERR_packet(buffer, size, BAD_OACK, "Block size larger than specified by client.");
            sendto(socket_fd, buffer, size, 0, address, addr_length);
            return true;
        }
    }

    if (data.timeout != 0)
    {
        if (negotiation.timeout == 0) // server neodpovedel na timeout 
        {
            cerr << "Warning: Server did not recognize timeout option. Client timeout remains at " << (uint)data.timeout << " s." << endl;
        }
        else if (data.timeout != 0 && negotiation.timeout != data.timeout) // server odpovedel jinou hodnotou timeout
        {
            set_timeout(socket_fd, negotiation.timeout); // akceptace timeout specifikovaneho serverem
            cerr << "Warning: Server did not accept specified timeout of " << (uint)data.timeout 
                << " s. Timeout specified by server of " << (uint)negotiation.timeout <<" s is used instead." << endl;
        }
    }

    if (data.mode == BINARY)
    {
        if (negotiation.transfer_size == -1) // server neodpovedel na velikost souboru
        {
            cerr << "Warning: Server did not recognize transfer size option. Transfer continues." << endl;
        }
        else if (negotiation.transfer_size > available_space()) // kontrola volneho mista na dsku
        {
            cerr << "Error: Transfered file is larger than available disk space." << endl;
            ERR_packet(buffer, size, DISK_FULL, "File size exeeds available disk space.");
            sendto(socket_fd, buffer, size, 0, address, addr_length);
            return true;
        }
    }

    return false;
}
