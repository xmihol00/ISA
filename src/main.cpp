#include "main.h"

int main()
{
    arguments_t arguments;

    cout << "> ";
    for (string line; getline(cin, line); )
    {
        if (parse_line(line, arguments))
        {
            cout << "file recieved" << endl;
        }
        cout << "> ";
    }

    return 0;
}
