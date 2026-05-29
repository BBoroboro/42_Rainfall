#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void p(void) {
    unsigned int retaddr;
    char buffer[76];

    fflush(stdout);
    gets(buffer);

    if ((retaddr & 0xB0000000) == 0xB0000000) {
        printf("(%p)\n", (void*)retaddr);
        _exit(1);
    }

    puts(buffer);
    strdup(buffer);
}

int main(void) {
    p();
    return 0;
}