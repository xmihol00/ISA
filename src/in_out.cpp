//====================================================================================================================
// Soubor:      in_out.cpp
// Projekt:     VUT, FIT, ISA, TFTPv2 klient
// Datum:       24. 10. 2021
// Autor:       David Mihola
// Kontakt:     xmihol00@stud.fit.vutbr.cz
// Kompilovano: gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
// Popis:       Soubor defunujici funkce pouzite pro ziskavani dat od uzivatele a vypis vysledku prenosu souboru.
//====================================================================================================================

#include "in_out.h"

void help_msg()
{
    cout << "Command prompt usage:" << endl;
    cout << "   -R                               : Compulsory, cannot be combined with '-W' option. Specifies reading a file from server." << endl;
    cout << "   -W                               : Compulsory, cannot be combined with '-R' option. Specifies writing a file to server." << endl;
    cout << "   -d <filepath/filename>           : Compulsory. Specifies the path to a read or written file and its name." << endl;
    cout << "   -t <0-255 inclusive>             : Optional. Specifies the used timeout in seconds before potentionaly lost datagram" << endl;
    cout << "                                      is resend. The default value is 1 s." << endl;
    cout << "   -s <1-2147483647 inclusive>      : Optional. Specifies the used block size of transported data in one datagram." << endl; 
    cout << "                                      Larger values may be replaced with the MTU of the system." << endl;
    cout << "   -m                               : Optional. Specifies the use of multicast for reading a file from server." << endl;
    cout << "                                      When '-W' option is specified, the value is ignored." << endl;
    cout << "   -c <netascii|ascii|binary|octet> : Optional. Specifies the used encoding of transfered data. Default value is 'binary'." << endl;
    cout << "   -a <IP address,port>             : Optional. Specifies the IPv4 or IPv6 address and port, on which the server is listening." << endl;
    cout << "                                      Default values are '127.0.0.1,69'." << endl;
    cout << "   q                                : Safely termiantes the application." << endl;
    cout << "   exit                             : Safely termiantes the application." << endl;
}

int parse_line(const string &line, arguments_t &arguments)
{
    if (line == "q" || line == "exit") // vypnuti apliakce
    {
        return -1;
    }
    else if (line == "?" || line == "h") // napoveda
    {
        help_msg();
        return 0;
    }
    else if (line == "") // pouze enter
    {
        return 0;
    }

    bool mode = false;
    long converter = 0;
    char *endptr = nullptr;

    // nastaveni defaultnich hodnot
    arguments.file_URL = string();
    arguments.address_type = UNSET;
    arguments.data_mode = BINARY;
    arguments.port = htons(DEFAULT_PORT);
    arguments.timeout = 0;
    arguments.multicast = false;
    arguments.block_size = 512;

    istringstream stream(line);
    for (string word; stream >> word;) // prochazeni slov ve vstupnim radku
    {
        if (word == "-R") // cteni
        {
            if (mode)
            {
                cerr << "Error: Mode of file transfer specified more than once. Type '?' or 'h' for help." << endl;
                return 0;
            }
            mode = true;
            arguments.transfer_mode = READ;
        }
        else if (word == "-W") // zapis
        {
            if (mode)
            {
                cerr << "Error: Mode of file transfer specified more than once. Type '?' or 'h' for help." << endl;
                return 0;
            }
            mode = true;
            arguments.transfer_mode = WRITE;
        }
        else if (word == "-d") // cesta a jmeno souboru
        {
            if (!(stream >> arguments.file_URL)) // ziskani hodnoty
            {
                cerr << "Error: -d option requires a parameter. Type '?' or 'h' for help." << endl;
                return 0;
            }
            if (arguments.file_URL.size() > MAX_URL_LEN) // buffer overflow may occure (buffer size is 1024 B)
            {
                cerr << "Error: lenght of the file URL cannot be larger than 512 characters. Type '?' or 'h' for help." << endl;
                return 0;
            }
        }
        else if (word == "-t") // timeout
        {
            if (!(stream >> word)) // ziskani hodnoty
            {
                cerr << "Error: -t option requires a parameter. Type '?' or 'h' for help." << endl;
                return 0;
            }
            // prevod na cislo a kontrola rozsahu
            converter = strtol(word.c_str(), &endptr, 10);
            if (converter < 0L || converter > 255L)
            {
                cerr << "Error: Timeout out of range. Type '?' or 'h' for help." << endl;
                return 0;
            }
            else if (endptr != nullptr && *endptr != '\0')
            {
                cerr << "Error: -t option must be followed by an integer between 1 and 255 inclusive. Type '?' or 'h' for help." << endl;
                return 0;
            }
            arguments.timeout = (uint8_t)converter;
        }
        else if (word == "-s") // velikost prenaseneho bloku
        {
            if (!(stream >> word))
            {
                cerr << "Error: -s option requires a parameter. Type '?' or 'h' for help." << endl;
                return 0;
            }
            // prevod na cislo a kontrola rozsahu
            converter = strtol(word.c_str(), &endptr, 10);
            if (converter < MIN_BLK_SIZE || converter > INT32_MAX)
            {
                cerr << "Error: Block size out of range. Type '?' or 'h' for help." << endl;
                return 0;
            }
            else if (endptr != nullptr && *endptr != '\0')
            {
                cerr << "Error: -s option must be followed by a positive integer. Type '?' or 'h' for help." << endl;
                return 0;
            }
            arguments.block_size = (int)converter;
        }
        else if (word == "-a") // adresa serveru
        {
            if (!(stream >> word)) // ziskani adresy
            {
                cerr << "Error: -a option requires a parameter. Type '?' or 'h' for help." << endl;
                return 0;
            }
            size_t pos = word.find(',');
            string address;
            if (pos != string::npos) // mezi IP adresou a carkou neni mezera
            {
                address = word.substr(0, pos);

                if (pos + 1 == word.size()) // mezi carkou a portem je mezera
                {
                    if (!(stream >> word))
                    {
                        cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                        return 0;
                    }
                }
                else // mezi carkou a portem neni mezera
                {
                    word = word.substr(pos + 1);
                }
            }
            else // mezi IP a carkou je mezera 
            {
                address = word;
                if (!(stream >> word)) // adresa neobsahuje carku na port
                {
                    cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                    return 0;
                }

                if (word == ",") // carka je samostatne
                {
                    if (!(stream >> word)) // chyby port
                    {
                        cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                        return 0;
                    }   
                }
                else // mezi carkou a portem neni mezera
                {
                    pos = word.find(",");
                    if (pos == string::npos) // slovo neobsahuje carku
                    {
                        cerr << "Error: Address must contain a port number. Type '?' or 'h' for help." << endl;
                        return 0;
                    }

                    word = word.substr(pos + 1); // ziskani portu
                }
            }
            
            // prevod adresy na byty
            if (!inet_pton(AF_INET, address.c_str(), &arguments.address.ipv4))
            {
                if (!inet_pton(AF_INET6, address.c_str(), &arguments.address.ipv6))
                {
                    cerr << "Error: Invalid IP address. Type '?' or 'h' for help." << endl;
                    return 0;
                }
                arguments.address_type = IPv6;
            }
            else
            {
                arguments.address_type = IPv4;
            }

            // prevod portu na cislo
            converter = strtol(word.c_str(), &endptr, 10);
            if (converter < 0L || converter > UINT16_MAX)
            {
                cerr << "Error: Port number out of range. Type '?' or 'h' for help." << endl;
                return 0;
            }
            else if (endptr != nullptr && *endptr != '\0')
            {
                cerr << "Error: Port number must be a positive integer. Type '?' or 'h' for help." << endl;
                return 0;
            }
            // prevod portu na spravny endian
            arguments.port = (in_port_t)htons(converter); 
        }
        else if (word == "-c") // typ kodovani prenasenych dat
        {
            if (!(stream >> word))
            {
                cerr << "Error: -c option requires a parameter. Type '?' or 'h' for help." << endl;
                return 0;
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
                return 0;
            }
        }
        else if (word == "-m") // mutlicast
        {
            arguments.multicast = true;
        }
        else // chyba
        {
            cerr << "Error: Unknown or incorrect option specified. Type '?' or 'h' for help." << endl;
            return 0;
        }
    }

    if (!mode) // chyby READ nebo WRITE - typ prenosu
    {
        cerr << "Error: Mode of file transfer not specified. Type '?' or 'h' for help." << endl;
        return 0;
    }

    if (arguments.file_URL.empty()) // chyby jmeno souboru
    {
        cerr << "Error: Transported file not specified. Type '?' or 'h' for help." << endl;
        return 0;
    }

    if (arguments.address_type == UNSET) // nastaveni defaultno adresy, pokud zadna nebyla zadana
    {
        arguments.address_type = IPv4;
        inet_pton(AF_INET, "127.0.0.1", &arguments.address.ipv4);
    }

    return 1;
}

string get_file_name(const string &file_URL)
{
    size_t pos = file_URL.find_last_of("/");
    if (pos == string::npos)
    {
        return file_URL;
    }
    else
    {
        return file_URL.substr(pos + 1);
    }
}

long available_space()
{
    struct statvfs stats;
    if (statvfs(".", &stats) == -1)
    {
        cerr << "Warning: Could not obtain available disk space." << endl;
    }

    return stats.f_bsize * stats.f_bavail; // pocet volnych bloku * velikost bloku
}

ssize_t fread_to_netascii(FILE *file, ssize_t block_size, char *buffer)
{
    static int last = -1;
    ssize_t i = 0;
    int c = 0;
    if (last != -1)
    {
        buffer[i++] = last;
    }
    
    for (; i < block_size && (c = fgetc(file)) != EOF; i++)
    {
        if (c == LF)
        {
            buffer[i++] = CR;
            if (i == block_size)
            {
                last = LF;
                return i;
            }
        }

        if (c == CR)
        {
            c = '\0';
            buffer[i++] = CR;
            if (i == block_size)
            {
                last = c;
                return i;
            }
        }

        buffer[i] = c;
    }
    
    last = -1;
    return i;
}

bool fwrite_from_netascii(FILE *file, ssize_t size, char *buffer)
{
    static int last = -1;
    for (ssize_t i = 0; i < size; i++)
    {
        if (buffer[i] == CR)
        {
            last = CR;
            continue;
        }

        if (last == CR)
        {
            if (buffer[i] == LF)
            {
                putc('\n', file);
            }
            else if (buffer[i] == '\0')
            {
                putc(CR, file);
            }
            else
            {
                return true;
            }
        }
        else
        {
            putc(buffer[i], file);
        }

        last = buffer[i];
    }

    return false;
}

void print_summary(const transfer_summary_t &summary, system_clock::time_point start, system_clock::time_point end)
{
    cout << endl;
    if (summary.success) // prevod souboru uspesny
    {
        if (summary.mode == READ) // cteni
        {
            cout << "Reading file '" << summary.file << "' succeeded." << endl;

            cout << "Transfer summary:" << endl;
            cout << "   - " << (end - start) / milliseconds(1) << " ms elapsed during the transfer," << endl;
            cout << "   - " << summary.data_size << " B of data were recieved" << " in " 
                            << summary.datagram_count << " datagram" << (summary.datagram_count > 1 ? "s" : "") << " of a maximum data size " 
                            << summary.blksize << " B" << (summary.lost_count > 0 ? "," : ".") << endl;
            
            if (summary.lost_count > 0) // ztrata datagramu
            {
                cout << "     additional " << summary.lost_count << " datagram" << (summary.lost_count > 1 ? "s" : "") 
                     << " carrying data may have been lost, which compounds up to " << summary.lost_size << " B." << endl;
            }
        }
        else // zapis 
        {
            cout << "Writing file '" << summary.file << "' succeeded." << endl;

            cout << "Transfer summary:" << endl;
            cout << "   - " << (end - start) / milliseconds(1) << " ms elapsed during the transfer," << endl;
            cout << "   - " << summary.data_size << " B of data were sent" << " in " 
                            << summary.datagram_count << " datagram" << (summary.datagram_count > 1 ? "s" : "") << " of a maximum data size " 
                            << summary.blksize << " B" << (summary.lost_count > 0 ? "," : ".") << endl;
            
            if (summary.lost_count > 0) // ztrata datagramu
            {
                cout << "     of which " << summary.lost_count << " datagram" << (summary.lost_count > 1 ? "s" : "") 
                     << " carrying data may have been lost of overall data size " << summary.lost_size << " B." << endl;
            }
        }
    }
    else // prevod souboru neuspesny
    {
        cout << (summary.mode == READ ? "Reading file '" : "Writing file '") << summary.file << "' failed." << endl;

        cout << "Transfer summary:" << endl;
        cout << "   - " << (end - start) / milliseconds(1) << " ms elapsed before failure," << endl;
        cout << "   - " << summary.data_size << " B including potential errror message " << (summary.mode == READ ? "recieved" : "sent") 
                        << " before failure occured in " << summary.datagram_count << " datagram" 
                        << (summary.datagram_count != 1 ? "s" : "") << " of a maximum data size " << summary.blksize << " B." << endl;   
    }    
    cout << endl;
}
