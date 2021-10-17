
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
    time_out.tv_sec = 5;
    time_out.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(time_out));    

    read(socket_fd, address, address_len, arguments.file_URL, arguments.data_mode);

    close(socket_fd);
}

void read(int socket_fd, struct sockaddr *address, socklen_t length, const string &file_URL, data_mode_t mode)
{
    cerr << length << endl;
    char buffer[BUFFER_ZIZE];
    ssize_t size;
    RRQ_header(buffer, file_URL, mode, size);
    if (sendto(socket_fd, buffer, size, 0, address, length) != size)
    {
        cerr << "Error: Transfer of data failed." << endl;
        return;
    }

    string file_name;
    size_t pos = file_URL.find_last_of("/");
    if (pos == string::npos)
    {
        file_name = file_URL;
    }
    else
    {
        file_name = file_URL.substr(pos + 1);
    }

    FILE *file = fopen(file_name.c_str(), "w");

    struct sockaddr_in6 server;
    struct sockaddr *recieve_address = (struct sockaddr *) &server;
    getsockname(socket_fd, recieve_address, &length);
    size = recvfrom(socket_fd, buffer, BUFFER_ZIZE, 0, recieve_address, &length);
    if (size == -1)
    {
        cerr << "Error: Transfer of data failed." << endl;
        fclose(file);
        return;
    }
    ((sockaddr_in *)address)->sin_port = server.sin6_port;

    buffer[516] = 0;
    cerr << &buffer[4];
    fwrite(&buffer[4], 1, size - 4, file);
    while (size == 516)
    {
        ACK_header(buffer, size);
        if (sendto(socket_fd, buffer, size, 0, address, length) != size)
        {
            cerr << "Error: Transfer of data failed." << endl;
        }

        size = recvfrom(socket_fd, buffer, BUFFER_ZIZE, 0, recieve_address, &length);
        buffer[516] = 0;
        fwrite(&buffer[4], 1, size - 4, file);
        cerr << &buffer[4];
    }
    if (size != -1)
    {
        ACK_header(buffer, size);
        if (sendto(socket_fd, buffer, size, 0, address, length) != size)
        {
            cerr << "Error: Transfer of data failed." << endl;
        }
    }
    else
    {
        cerr << "Error: Transfer of data failed." << endl;
    }

    fclose(file);
};
