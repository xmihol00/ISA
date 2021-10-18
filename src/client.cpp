
#include "client.h"

void transfer(const arguments_t &arguments)
{
    struct sockaddr *destination;
    socklen_t destination_len;
    struct timeval time_out;
    int socket_fd, mtu;

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

    mtu = get_MTU(*destination, destination_len);
    cerr << "MTU: " << mtu << endl;

    time_out.tv_sec = arguments.timeout;
    time_out.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out));    

    if (arguments.transfer_mode == READ)
    {
        read(socket_fd, destination, destination_len, arguments.file_URL, arguments.data_mode);
    }
    else
    {
        write(socket_fd, destination, destination_len, arguments.file_URL, arguments.data_mode);
    }

    close(socket_fd);
}

void read(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode)
{
    uint8_t retries = 0;

    char buffer[BUFFER_SIZE];
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];

    char ack_buff[ACK_BUFFER_SIZE];
    uint16_t *ack_block_number = (uint16_t *)&ack_buff[2];

    ssize_t size;
    struct sockaddr_in6 server;
    server.sin6_port = ((sockaddr_in *)address)->sin_port;
    struct sockaddr *recieve_address = (struct sockaddr *) &server;
    
    string file_name = get_file_name(file_URL);
    FILE *file = fopen(file_name.c_str(), "w");
    if (file == nullptr)
    {
        cerr << "Error: Could not open file '" << file_name << "' for writing." << endl;
        return;
    }

    do
    {
        ((sockaddr_in *)address)->sin_port = server.sin6_port;
        
        if (retries++ > MAX_RETRIES)
        {
            cerr << "Error: Transfer of data failed. Server timed out." << endl;
            goto file_disposal;
        }

        RRQ_header(buffer, file_URL, mode, size);
        if (sendto(socket_fd, buffer, size, 0, address, length) != size)
        {
            cerr << "Error: Transfer of data failed: " << strerror(errno) << endl;
            goto file_disposal;
        }
        getsockname(socket_fd, recieve_address, &length);
    }
    while ((size = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, recieve_address, &length)) == -1);
    retries = 0;

    if (*opcode != DATA)
    {
        if (*opcode == ERR)
        {
            cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[4] << endl;
            goto file_disposal;
        }
        else
        {
            cerr << "Error: Transfer of data failed. Malformed packet recieved." << endl;
            goto file_disposal;
        }
    }
    ((sockaddr_in *)address)->sin_port = server.sin6_port;

    buffer[516] = 0;
    fwrite(&buffer[4], 1, size - 4, file);
    while (size == TFTP_DATAGRAM_SIZE)
    {
        do
        {
            if (retries++ > MAX_RETRIES)
            {
                cerr << "Error: Transfer of data failed. Server timed out." << endl;
                goto file_disposal;
            }
            ACK_header(ack_buff, *block_number, size);
            if (sendto(socket_fd, ack_buff, size, 0, address, length) != size)
            {
                cerr << "Error: Transfer of data failed: " << strerror(errno) << endl;
                goto file_disposal;
            }
        } 
        while ((size = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, recieve_address, &length)) == -1 || 
                (ntohs(*block_number) != ntohs(*ack_block_number) + 1 && *opcode == DATA));

        if (*opcode != DATA)
        {
            if (*opcode == ERR)
            {
                cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[4] << endl;
                goto file_disposal;
            }
            else
            {
                cerr << "Error: Transfer of data failed. Malformed packet recieved." << endl;
                goto file_disposal;
            }
        }   

        retries = 0;
        buffer[516] = 0;
        fwrite(&buffer[4], 1, size - 4, file);
    }
    ACK_header(buffer, *block_number, size);
    sendto(socket_fd, buffer, size, 0, address, length);

file_disposal:
    fclose(file);
};

void write(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode)
{
    uint8_t retries = 0;
    
    char buffer[BUFFER_SIZE];
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *block_number = (uint16_t *)&buffer[2];

    char ack_buff[ACK_BUFFER_SIZE];
    uint16_t *ack_opcode = (uint16_t *)ack_buff;
    uint16_t *ack_block_number = (uint16_t *)&ack_buff[2];

    ssize_t size;
    struct sockaddr_in6 server;
    server.sin6_port = ((sockaddr_in *)address)->sin_port;
    struct sockaddr *recieve_address = (struct sockaddr *) &server;
    
    string file_name = get_file_name(file_URL);
    FILE *file = fopen(file_name.c_str(), "r");
    if (file == nullptr)
    {
        cerr << "Error: Could not open file '" << file_name << "' for reading." << endl;
        return;
    }

    do
    {
        ((sockaddr_in *)address)->sin_port = server.sin6_port;
        
        if (retries++ > MAX_RETRIES)
        {
            cerr << "Error: Transfer of data failed. Server timed out." << endl;
            goto file_disposal;
        }

        WRQ_header(buffer, file_URL, mode, size);
        if (sendto(socket_fd, buffer, size, 0, address, length) != size)
        {
            cerr << "Error: Transfer of data failed: " << strerror(errno) << endl;
            goto file_disposal;
        }
        getsockname(socket_fd, recieve_address, &length);
    } 
    while ((size = recvfrom(socket_fd, ack_buff, ACK_BUFFER_SIZE, 0, recieve_address, &length)) == -1 || *ack_block_number != 0);
    ((sockaddr_in *)address)->sin_port = server.sin6_port;
    retries = 0;

    if (*ack_opcode != ACK)
    {
        if (*ack_opcode == ERR)
        {
            cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[4] << endl;
            goto file_disposal;
        }
        else
        {
            cerr << "Error: Transfer of data failed. Malformed packet recieved." << endl;
            goto file_disposal;
        }
    }

    for (uint16_t i = 1; ; i++)
    {
        *opcode = DATA;
        *block_number = htons(i);
        size = fread(&buffer[4], 1, TFTP_DATA_SIZE, file);
        size += 4;

        do
        {
            if (retries++ > MAX_RETRIES)
            {
                cerr << "Error: Transfer of data failed. Server timed out." << endl;
                goto file_disposal;
            }

            if (sendto(socket_fd, buffer, size, 0, address, length) != size)
            {
                cerr << "Error: Transfer of data failed: " << strerror(errno) << endl;
                goto file_disposal;
            }
        }
        while (recvfrom(socket_fd, ack_buff, ACK_BUFFER_SIZE, 0, recieve_address, &length) == -1 || 
                        (*ack_block_number != *block_number && *ack_opcode == ACK)); 
        retries = 0;
        
        if (*ack_opcode != ACK)
        {
            if (*ack_opcode == ERR)
            {
                cerr << "Error: " << err_code_value(*block_number) << " Message: " << &buffer[4] << endl;
                goto file_disposal;
            }
            else
            {
                cerr << "Error: Transfer of data failed. Malformed packet recieved." << endl;
                goto file_disposal;
            }
        }

        if (size < TFTP_DATAGRAM_SIZE)
        {
            break;
        }
    }

file_disposal:
    fclose(file);
}

int get_MTU(sockaddr destination, socklen_t len)
{
    struct ifaddrs *address = nullptr, *head = nullptr;
    int socket_fd;
    struct ifreq request;

    if ((socket_fd = socket(destination.sa_family, SOCK_DGRAM, 0)) == -1)
    {
        cerr << "Error: Could not create socket." << endl;
    }

    if (connect(socket_fd, &destination, len) == -1)
    {
        cerr << "Error: Could not connect socket." << endl;
        goto failed;
    }

    if (getsockname(socket_fd, &destination, &len) == -1)
    {
        cerr << "Error: Could not obtain socket information." << endl;
        goto failed;
    }

    if (getifaddrs(&head) == -1)
    {
        cerr << "Error: could not obtain interface addresses" << endl;
        goto failed;
    }

    for (address = head; address != nullptr; address = address->ifa_next)
    {
        if (address->ifa_addr != nullptr && address->ifa_addr->sa_family == destination.sa_family)
        {
            strncpy(request.ifr_name, address->ifa_name, sizeof(request.ifr_name));
            if (ioctl(socket_fd, SIOCGIFADDR, &request) == -1)
            {
                cerr << "Error: Could not obtain interface IP address." << endl;
                goto failed;
            }

            if (((struct sockaddr_in *)&request.ifr_addr)->sin_addr.s_addr == ((struct sockaddr_in *)&destination)->sin_addr.s_addr)
            {
                if (ioctl(socket_fd, SIOCGIFMTU, &request) == -1)
                {
                    cerr << "Error: Could not obtain interface MTU." << endl;
                    goto failed;
                }
                break;
            }
        }
    }

    freeifaddrs(head);
    return request.ifr_mtu;

failed:
    freeifaddrs(head);
    return -1;
}
