
#include "client.h"

void transfer(const arguments_t &arguments)
{
    struct sockaddr *destination;
    socklen_t destination_len;
    int socket_fd;
    int32_t mtu;

    if (arguments.address_type == IPv4)
    {
        struct sockaddr_in server_v4;
        server_v4.sin_addr = arguments.address.ipv4;
        server_v4.sin_family = AF_INET;
        server_v4.sin_port = arguments.port;
        destination = (struct sockaddr *)&server_v4;
        destination_len = sizeof(server_v4);
        socket_fd = socket(AF_INET , SOCK_DGRAM , 0);
    }
    else
    {
        struct sockaddr_in6 server_v6;
        server_v6.sin6_addr = arguments.address.ipv6;
        server_v6.sin6_family = AF_INET6;
        server_v6.sin6_port = arguments.port;
        destination = (struct sockaddr *)&server_v6;
        destination_len = sizeof(server_v6);
        socket_fd = socket(AF_INET6 , SOCK_DGRAM , 0);
    }
    if (socket_fd == -1)
    {
        cerr << "Error: Socket could no be created." << endl;
        return;
    }

    if ((mtu = get_min_MTU(*destination)) == INT32_MAX)
    {
        cerr << "Warning: MTU could not be obtained, default block size of 512 is used instead." << endl;
        mtu = TFTP_DATA_SIZE;
    }
    else
    {
        if (destination->sa_family == AF_INET)
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
        else
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

    set_timeout(socket_fd, arguments.timeout);

    if (arguments.transfer_mode == READ)
    {
        read(socket_fd, destination, destination_len, {arguments.file_URL, arguments.data_mode, mtu, arguments.multicast, arguments.timeout});
    }
    else
    {
        write(socket_fd, destination, destination_len, {arguments.file_URL, arguments.data_mode, mtu, arguments.multicast, arguments.timeout});
    }

    close(socket_fd);
}

void read(int socket_fd, struct sockaddr *address, socklen_t addr_length, transfer_data_t data)
{
    uint8_t retries = 0;
    in_port_t TID;
    TFTP_options_t options = { .file_URL = data.file_URL, .opcode = RRQ, .mode = data.mode, .block_size = data.block_size, 
                               .transfer_size = 0, .timeout = data.timeout, .multicast = data.multicast };
    

    char *buffer = new char[data.block_size > TFTP_DATA_SIZE ? data.block_size + TFTP_HDR : TFTP_DATAGRAM_SIZE];
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];
    uint16_t ack_number;

    char ack_buff[ACK_BUFFER_SIZE] = {0, };

    ssize_t size;
    struct sockaddr_in6 recieved_address;
    recieved_address.sin6_port = ((sockaddr_in *)address)->sin_port;
    struct sockaddr *ptr_recieved_address = (struct sockaddr *) &recieved_address;
    socklen_t recv_lenght = sizeof(recieved_address);
    
    string file_name = get_file_name(data.file_URL);
    FILE *file = fopen(file_name.c_str(), "w");
    if (file == nullptr)
    {
        cerr << "Error: Could not open file '" << file_name << "' for writing." << endl;
        goto data_cleanup;
    }

    do
    {
        ((sockaddr_in *)address)->sin_port = recieved_address.sin6_port;
        
        if (retries++ > MAX_RETRIES)
        {
            cerr << "Error: Transfer of data failed. Server timed out." << endl;
            goto cleanup;
        }

        RQ_header(buffer, size, options);
        if (sendto(socket_fd, buffer, size, 0, address, addr_length) != size)
        {
            cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
            goto cleanup;
        }
    }
    while ((size = recvfrom(socket_fd, buffer, data.block_size + TFTP_HDR, 0, ptr_recieved_address, &recv_lenght)) == -1 || 
           (*opcode == DATA && ntohs(*block_number) != 1));
    retries = 0;
    ((sockaddr_in *)address)->sin_port = TID = recieved_address.sin6_port;
    ack_number = *block_number;

    if (*opcode == OACK)
    {
        if (set_negotioation(socket_fd, address, addr_length, buffer, size, data))
        {
            goto cleanup;
        }

        while(true)
        {
            do
            {
                if (retries++ > MAX_RETRIES)
                {
                    cerr << "Error: Transfer of data failed. Server timed out." << endl;
                    goto cleanup;
                }

                ACK_header(buffer, size, 0);
                if (sendto(socket_fd, buffer, size, 0, address, addr_length) != size)
                {
                    cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
                    goto cleanup;
                }
            }
            while ((size = recvfrom(socket_fd, buffer, data.block_size, 0, ptr_recieved_address, &recv_lenght)) == -1 ||
                   (recieved_address.sin6_port == TID && ntohs(*block_number) != 1 && *opcode == DATA));

            if (recieved_address.sin6_port != TID)
            {
                cerr << "Warning: Recieved TID does not match the estabhlished one, transfer continues." << endl;
                ERR_packet(ack_buff, size, UNKNOWN_ID, "Incorrect TID.");
                sendto(socket_fd, ack_buff, size, 0, address, addr_length);
                retries--;
                continue;
            }

            retries = 0;
            break;
        }
        ack_number = htons(1);
    }
    else
    {
        cerr << "Warning: Server does not support Option Negotiation." << endl;
        data.block_size = TFTP_DATAGRAM_SIZE;
    }

    if (*opcode != DATA)
    {
        if (*opcode == ERR)
        {
            cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[TFTP_HDR] << endl;
            goto cleanup;
        }
        else
        {
            cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
            goto cleanup;
        }
    }

    fwrite(&buffer[TFTP_HDR], 1, size - TFTP_HDR, file);
    while (size == data.block_size)
    {
        while(true)
        {
            do
            {
                if (retries++ > MAX_RETRIES)
                {
                    cerr << "Error: Transfer of data failed. Server timed out." << endl;
                    goto cleanup;
                }
                ACK_header(ack_buff, size, ack_number);
                if (sendto(socket_fd, ack_buff, size, 0, address, addr_length) != size)
                {
                    cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
                    goto cleanup;
                }
            } 
            while ((size = recvfrom(socket_fd, buffer, data.block_size, 0, ptr_recieved_address, &recv_lenght)) == -1 || 
                   (recieved_address.sin6_port == TID && ntohs(*block_number) != ntohs(ack_number) + 1 && *opcode == DATA));
            
            if (recieved_address.sin6_port != TID)
            {
                cerr << "Warning: Recieved TID does not match the estabhlished one, transfer continues." << endl;
                ERR_packet(ack_buff, size, UNKNOWN_ID, "Incorrect TID.");
                sendto(socket_fd, ack_buff, size, 0, address, addr_length);
                continue;
            }

            ack_number = *block_number;
            break;
        }

        if (*opcode != DATA)
        {
            if (*opcode == ERR)
            {
                cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[TFTP_HDR] << endl;
                goto cleanup;
            }
            else
            {
                cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
                goto cleanup;
            }
        }   

        retries = 0;
        fwrite(&buffer[TFTP_HDR], 1, size - TFTP_HDR, file);
    }
    ACK_header(buffer, size, *block_number);
    sendto(socket_fd, buffer, size, 0, address, addr_length);

cleanup:
    fclose(file);
data_cleanup:
    delete[] buffer;
};

void write(int socket_fd, struct sockaddr *address, socklen_t addr_length, transfer_data_t data)
{
    uint8_t retries = 0;
    in_port_t TID = 0;
    TFTP_options_t options = { .file_URL = data.file_URL, .opcode = WRQ, .mode = data.mode, .block_size = data.block_size, 
                               .transfer_size = 0, .timeout = data.timeout, .multicast = data.multicast };
    
    char *buffer = new char[data.block_size > TFTP_DATA_SIZE ? data.block_size + TFTP_HDR : TFTP_DATAGRAM_SIZE];
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];

    char ack_buff[ACK_BUFFER_SIZE] = {0, };
    uint16_t *ack_opcode = (uint16_t *)ack_buff;
    uint16_t *ack_block_number = (uint16_t *)&ack_buff[2];

    ssize_t size = 0;
    ssize_t err_size = ACK_BUFFER_SIZE;
    struct sockaddr_in6 recieved_address;
    recieved_address.sin6_port = ((sockaddr_in *)address)->sin_port;
    struct sockaddr *ptr_recieved_address = (struct sockaddr *) &recieved_address;
    socklen_t recv_lenght = sizeof(recieved_address);
    
    string file_name = get_file_name(data.file_URL);
    FILE *file = fopen(file_name.c_str(), "r");
    if (file == nullptr)
    {
        cerr << "Error: Could not open file '" << file_name << "' for reading." << endl;
        goto data_cleanup;
    }
    if (fseek(file, 0, SEEK_END) == -1 || (options.transfer_size = ftell(file)) == -1 || fseek(file, 0, SEEK_SET) == -1)
    {
        rewind(file);
        cerr << "Warning: Could not obtain size of file '" << file_name << "', 'tsize' option will not be used." << endl;
        options.transfer_size = -1;
    }

    do
    {
        ((sockaddr_in *)address)->sin_port = recieved_address.sin6_port;
        
        if (retries++ > MAX_RETRIES)
        {
            cerr << "Error: Transfer of data failed. Server timed out." << endl;
            goto cleanup;
        }

        RQ_header(buffer, size, options);
        if (sendto(socket_fd, buffer, size, 0, address, addr_length) != size)
        {
            cerr << "Error: Transfer of data failed. Data could not be send fully.";
            goto cleanup;
        }
    } 
    while ((size = recvfrom(socket_fd, ack_buff, ACK_BUFFER_SIZE, 0, ptr_recieved_address, &recv_lenght)) == -1 || 
           (*ack_opcode == ACK && *ack_block_number != 0));
    
    ((sockaddr_in *)address)->sin_port = TID = recieved_address.sin6_port;
    retries = 0;

    if (*ack_opcode == OACK)
    {
        if (set_negotioation(socket_fd, address, addr_length, ack_buff, size, data))
        {
            goto cleanup;
        }
    }
    else if (*ack_opcode == ACK)
    {
        cerr << "Warning: Server does not support Option Negotiation." << endl;
        data.block_size = TFTP_DATA_SIZE;
    }
    else if (*ack_opcode == ERR) 
    {
        
        cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[TFTP_HDR] << endl;
        goto cleanup;
    }
    else
    {
        cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
        // TODO err
        goto cleanup;
    }

    for (uint16_t i = 1; ; i++)
    {
        *opcode = DATA;
        *block_number = htons(i);
        size = fread(&buffer[TFTP_HDR], 1, data.block_size, file);
        size += TFTP_HDR;

        while(true)
        {
            do
            {
                if (retries++ > MAX_RETRIES)
                {
                    cerr << "Error: Transfer of data failed. Server timed out." << endl;
                    goto cleanup;
                }

                if (sendto(socket_fd, buffer, size, 0, address, addr_length) != size)
                {
                    cerr << "Error: Transfer of data failed. Data could not be send fully." << endl;
                    goto cleanup;
                }
            }
            while (recvfrom(socket_fd, ack_buff, ACK_BUFFER_SIZE, 0, ptr_recieved_address, &recv_lenght) == -1 || 
                   (recieved_address.sin6_port == TID && *ack_block_number != *block_number && *ack_opcode == ACK)); 

            if (recieved_address.sin6_port != TID)
            {
                cerr << "Warning: Recieved TID does not match the estabhlished one, transfer continues." << endl;
                ERR_packet(ack_buff, err_size, UNKNOWN_ID, "Incorrect TID.");
                sendto(socket_fd, ack_buff, err_size, 0, address, addr_length);
                retries--;
                continue;
            }

            retries = 0;
            break;
        } 
        
        if (*ack_opcode != ACK)
        {
            if (*ack_opcode == ERR)
            {
                cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[TFTP_HDR] << endl;
                goto cleanup;
            }
            else
            {
                cerr << "Error: Transfer of data failed. Unexpected packet recieved." << endl;
                goto cleanup;
            }
        }

        if (size < data.block_size + TFTP_HDR)
        {
            break;
        }
    }

cleanup:
    fclose(file);
data_cleanup:
    delete[] buffer;
}

void set_timeout(int socket_fd, uint8_t timeout)
{
    struct timeval time_out;
    time_out.tv_sec = timeout == 0 ? DEFAULT_TIMEOUT : timeout;
    time_out.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out)) == -1)
    {
        cerr << "Warning: Timeout could not be set, program might freeze. Use Ctrl+C to terminate." << endl;
    }
}

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

    if (getsockname(socket_fd, destination, &lenght) == -1)
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
            if (ioctl(socket_fd, SIOCGIFADDR, &request) == -1)
            {
                continue;
            }

            if (request.ifr_addr.sa_family == destination->sa_family)
            {
                if ((destination->sa_family == AF_INET && 
                    ((struct sockaddr_in *)&request.ifr_addr)->sin_addr.s_addr == ((struct sockaddr_in *)destination)->sin_addr.s_addr) ||
                    (destination->sa_family == AF_INET6 && 
                    memcmp(&((struct sockaddr_in6 *)&request.ifr_addr)->sin6_addr, &((struct sockaddr_in6 *)destination)->sin6_addr, sizeof(struct in6_addr))))
                {
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

failed:
    close(socket_fd);
    freeifaddrs(head);
    return mtu;
}

int32_t get_min_MTU(sockaddr destination)
{
    int socket_fd;
    int32_t mtu = INT32_MAX;
    struct ifaddrs *current = nullptr, *head = nullptr;
    struct ifreq request;
    
    if ((socket_fd = socket(destination.sa_family, SOCK_DGRAM, 0)) == -1)
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
            if (ioctl(socket_fd, SIOCGIFMTU, &request) == -1)
            {
                continue;
            }
            
            if (mtu > request.ifr_mtu)
            {
                mtu = request.ifr_mtu;
            }
        }
    }

failed:
    close(socket_fd);
    freeifaddrs(head);
    return mtu;
}

bool set_negotioation(int socket_fd, struct sockaddr *address, socklen_t addr_length, char *buffer, ssize_t size, transfer_data_t &data)
{
    negotiation_t negotiation;
    
    try
    {
        negotiation = parse_OACK(buffer, size);
    }
    catch(const std::exception& e)
    {
        cerr << "Error: Could not parse datagram." << endl;
        ERR_packet(buffer, size, ILLEGAL_TFTP, "Could no parse OACK datagram.");
        sendto(socket_fd, buffer, size, 0, address, addr_length);
        return true;
    }

    if (negotiation.block_size == -1)
    {
        cerr << "Warning: Server did not recognize block size option. Default size of 512 B is used instead" << endl;
        data.block_size = TFTP_DATAGRAM_SIZE;
    }
    else if (negotiation.block_size <= data.block_size)
    {
        data.block_size = negotiation.block_size + TFTP_HDR;
    }
    else
    {
        cerr << "Error: Server specified block size larger than offered." << endl;
        ERR_packet(buffer, size, BAD_OACK, "Block size larger than specified by client.");
        sendto(socket_fd, buffer, size, 0, address, addr_length);
        return true;
    }

    if (data.timeout != 0 && negotiation.timeout == 0)
    {
        set_timeout(socket_fd, DEFAULT_TIMEOUT);
        cerr << "Warning: Server did not recognize timeout option. Default timeout of 1 s is used instead." << endl;
    }
    else if (data.timeout != 0 && negotiation.timeout != data.timeout)
    {
        set_timeout(socket_fd, DEFAULT_TIMEOUT);
        cerr << "Warning: Server did not accept specified timeout of " << data.timeout 
             << " s. Default timeout of 1 s is used instead." << endl;
    }

    if (negotiation.transfer_size == -1)
    {
        cerr << "Warning: Server did not recognize transfer size option. Transfer continues." << endl;
    }
    else if (negotiation.transfer_size > available_space())
    {
        cerr << "Error: Transfered file is larger than available disk space." << endl;
        ERR_packet(buffer, size, DISK_FULL, "File size exeeds available disk space.");
        sendto(socket_fd, buffer, size, 0, address, addr_length);
        return true;
    }

    return false;
}
