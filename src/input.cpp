#include "input.h"

void help_msg()
{
    cout << "Here will be a help message" << endl;
}

bool parse_line(const string &line, arguments_t &arguments)
{
    if (line == "q")
    {
        exit(0);
    }
    else if (line == "?" || line == "h")
    {
        help_msg();
        return false;
    }

    bool mode{false};
    long converter{0};
    char *endptr{nullptr};

    arguments.file_URL = string();
    arguments.address_type = UNSET;
    arguments.data_mode = BINARY;
    arguments.port = 69;
    arguments.timeout = 0;
    arguments.multicast = false;

    istringstream stream(line);
    for (string word; stream >> word;)
    {
        if (word == "-R")
        {
            if (mode)
            {
                cerr << "Error: Mode of file transfer specified more than once. Type '?' or 'h' for help." << endl;
                return false;
            }
            mode = true;
            arguments.transfer_mode = READ;
        }
        else if (word == "-W")
        {
            if (mode)
            {
                cerr << "Error: Mode of file transfer specified more than once. Type '?' or 'h' for help." << endl;
                return false;
            }
            mode = true;
            arguments.transfer_mode = WRITE;
        }
        else if (word == "-d")
        {
            if (!(stream >> arguments.file_URL))
            {
                cerr << "Error: -d option requires a parameter. Type '?' or 'h' for help." << endl;
                return false;
            }
        }
        else if (word == "-s")
        {
            if (!(stream >> word))
            {
                cerr << "Error: -s option requires a parameter. Type '?' or 'h' for help." << endl;
                return false;
            }
            converter = strtol(word.c_str(), &endptr, 10);
            if (converter < 0L || converter > UINT32_MAX)
            {
                cerr << "Error: Block size out of range. Type '?' or 'h' for help." << endl;
                return false;
            }
            else if (endptr != nullptr && *endptr != '\0')
            {
                cerr << "Error: -s option must be followed by a positive integer. Type '?' or 'h' for help." << endl;
                return false;
            }
            arguments.timeout = (uint32_t)converter;
        }
        else if (word == "-a")
        {
            if (!(stream >> word))
            {
                cerr << "Error: -a option requires a parameter. Type '?' or 'h' for help." << endl;
                return false;
            }
            size_t pos = word.find(',');
            string address;
            if (pos != string::npos)
            {
                address = word.substr(0, pos);

                if (pos + 1 == word.size())
                {
                    if (!(stream >> word))
                    {
                        cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                        return false;
                    }
                }
                else
                {
                    word = word.substr(pos + 1);
                }
            }
            else
            {
                address = word;
                if (!(stream >> word))
                {
                    cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                    return false;
                }

                if (word == ",")
                {
                    if (!(stream >> word))
                    {
                        cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                        return false;
                    }   
                }
                else
                {
                    pos = word.find(",");
                    if (pos == string::npos)
                    {
                        cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                        return false;
                    }

                    word = word.substr(pos + 1);
                }
            }
            
            if (!inet_pton(AF_INET, address.c_str(), &arguments.address.ipv4))
            {
                if (!inet_pton(AF_INET6, address.c_str(), &arguments.address.ipv6))
                {
                    cerr << "Error: Invalid IP address. Type '?' or 'h' for help." << endl;
                    return false;
                }
                arguments.address_type = IPv6;
            }
            else
            {
                arguments.address_type = IPv4;
            }

            converter = strtol(word.c_str(), &endptr, 10);
            if (converter < 0L || converter > UINT16_MAX)
            {
                cerr << "Error: Port number out of range. Type '?' or 'h' for help." << endl;
                return false;
            }
            else if (endptr != nullptr && *endptr != '\0')
            {
                cerr << "Error: Port number must be a positive integer. Type '?' or 'h' for help." << endl;
                return false;
            }
            arguments.port = (uint16_t)converter; 
        }
        else if (word == "-c")
        {
            if (!(stream >> word))
            {
                cerr << "Error: -c option requires a parameter. Type '?' or 'h' for help." << endl;
                return false;
            }

            if (word == "ascii" || word == "netascii")
            {
                arguments.data_mode = ASCII;
            }
            else if (word == "binary" || word == "octet")
            {
                arguments.data_mode = BINARY;
            }
            else
            {
                cerr << "Error: Unknown mode of data transfer. Type '?' or 'h' for help." << endl;    
                return false;
            }
        }
        else if (word == "-m")
        {
            arguments.multicast = true;
        }
        else
        {
            cerr << "Error: Unknown or incorrect option specified. Type '?' or 'h' for help." << endl;
            return false;
        }
    }

    if (!mode)
    {
        cerr << "Error: Mode of file transfer not specified. Type '?' or 'h' for help." << endl;
        return false;
    }

    if (arguments.file_URL.empty())
    {
        cerr << "Error: Transported file not specified. Type '?' or 'h' for help." << endl;
        return false;
    }

    if (arguments.address_type == UNSET)
    {
        arguments.address_type = IPv4;
        inet_pton(AF_INET, "127.0.0.1", &arguments.address.ipv4);
    }

    return true;
}
