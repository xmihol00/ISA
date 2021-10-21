#include "main.h"

int main()
{
    /*for (int i = 0; i < 256; i++)
    {
        putchar(i);
    }
    return 0;*/

    /*int c;
    while ((c = getc(stdin)) != EOF)
        printf("%d ", c);
    return 0;*/
    
    arguments_t arguments;

    cout << "> ";
    for (string line; getline(cin, line); )
    {
        if (parse_line(line, arguments))
        {
            transfer(arguments);
        }
        cout << "> ";
    }

    return 0;

}