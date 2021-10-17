
#include "client.h"

void transfer(const arguments_t &arguments)
{
    struct sockaddr *address;
    socklen_t address_len;
    int socket_fd;

    if (arguments.address_type == IPv4)
    {
        struct sockaddr_in server_v4;
        server_v4.sin_addr = arguments.address.ipv4;
        server_v4.sin_family = AF_INET;
        server_v4.sin_port = arguments.port;
        address = (struct sockaddr *)&server_v4;
        address_len = sizeof(server_v4);
        socket_fd = socket(AF_INET , SOCK_DGRAM , 0);
    }
    else
    {
        struct sockaddr_in6 server_v6;
        server_v6.sin6_addr = arguments.address.ipv6;
        server_v6.sin6_family = AF_INET6;
        server_v6.sin6_port = arguments.port;
        address = (struct sockaddr *)&server_v6;
        address_len = sizeof(server_v6);
        socket_fd = socket(AF_INET6 , SOCK_DGRAM , 0);
    }
    if (socket_fd == -1)
    {
        cerr << "Error: Socket could no be created." << endl;
        return;
    }

    struct timeval time_out;
    time_out.tv_sec = arguments.timeout;
    time_out.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out));    

    if (arguments.transfer_mode == READ)
    {
        read(socket_fd, address, address_len, arguments.file_URL, arguments.data_mode);
    }
    else
    {
        write(socket_fd, address, address_len, arguments.file_URL, arguments.data_mode);
    }

    close(socket_fd);
}

void read(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode)
{
    uint8_t retries = 0;
    char buffer[BUFFER_SIZE];
    uint16_t *opcode = (uint16_t *)buffer;
    uint16_t *err_code = (uint16_t *)&buffer[2];
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
            cerr << "Error: " << err_code_value(*err_code) << " Message: " << &buffer[4] << endl;
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
            ACK_header(buffer, size);
            if (sendto(socket_fd, buffer, size, 0, address, length) != size)
            {
                cerr << "Error: Transfer of data failed: " << strerror(errno) << endl;
                goto file_disposal;
            }
        } 
        while ((size = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, recieve_address, &length)) == -1);

        retries = 0;
        buffer[516] = 0;
        fwrite(&buffer[4], 1, size - 4, file);
        if (*opcode != DATA)
        {
            if (*opcode == ERR)
            {
                cerr << "Error: " << err_code_value(*err_code) << " Message: " << &buffer[4] << endl;
                goto file_disposal;
            }
            else
            {
                cerr << "Error: Transfer of data failed. Malformed packet recieved." << endl;
                goto file_disposal;
            }
        }   
    }
    ACK_header(buffer, size);
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
            cerr << "Error: Transfer of data failed. Server timed out.--" << endl;
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
        while (recvfrom(socket_fd, ack_buff, ACK_BUFFER_SIZE, 0, recieve_address, &length) == -1 || *ack_block_number != *block_number); 
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
