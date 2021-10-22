
#include <stdio.h>

int main ()
{
    for (int i = 0; i < 25; i++)
    {
        printf(":D ");
    }

    /*printf("!\n!:D");*/               // LF_end_77.txt
    /*printf("!\r!:D");*/               // CR_end_77.txt
    /*printf("\n!!:D");*/               // LF_1_to_end_77.txt
    /*printf("\r!!:D");*/               // CR_1_to_end_77.txt
    /*printf("!\r\n!:D");*/             // CRLF_split_end_77.txt 
    /*printf("!\n\r!:D");*/             // LFCR_split_end_77.txt
    /*printf("!\n!\r!\r\n!\n\r!:D");*/  // CR_LF_all.txt
    /*printf("!\n");*/                  // LF_last_77.txt
    /*printf("!\r");*/                  // CR_last_77.txt

    return 0;
}