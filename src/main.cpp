//====================================================================================================================
// Soubor:      main.cpp
// Projekt:     VUT, FIT, ISA, TFTPv2 klient
// Datum:       24. 10. 2021
// Autor:       David Mihola
// Kontakt:     xmihol00@stud.fit.vutbr.cz
// Kompilovano: gcc version 9.3.0 (Ubuntu 9.3.0-17ubuntu1~20.04)
// Popis:       Soubor s hlavni smyckou programu, ktera ocekava vstup uzivatele.
//====================================================================================================================

#include "main.h"

int main()
{
    arguments_t arguments;
    int status = 0;

    cout << "> ";
    for (string line; getline(cin, line); )
    {
        status = parse_line(line, arguments);
        if (status == 1)
        {
            transfer(arguments);
        }
        else if (status == -1)
        {
            break;
        }
        cout << "> ";
    }

    return 0;
}