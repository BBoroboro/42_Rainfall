#include <stdio.h>
#include <stdlib.h>

int m;

void v(void)
{
    char buffer[520];

    fgets(buffer, 0x200, stdin);

    printf(buffer);

    if (m == 0x40) {
        printf("Wait what?!\n");
        system("/bin/sh");
    }
}

int main(void)
{
    v();
    return 0;
}