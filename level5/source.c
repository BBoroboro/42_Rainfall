#include <stdio.h>
#include <stdlib.h>

void n(void) {
    char buffer[520];

    fgets(buffer, 0x200, stdin);
    printf(buffer);
    exit(1);
}

int main(void) {
    n();
    return 0;
}