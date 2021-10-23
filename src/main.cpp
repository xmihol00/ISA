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