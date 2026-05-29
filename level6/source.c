#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void m(void *param1, int param2, char *param3, int param4, int param5)
{
    puts("Nope");
}

int main(int argc, char **argv)
{
    char *dest;
    void (**func_ptr)();

    dest = malloc(0x40);
    func_ptr = malloc(sizeof(void (*)()));

    *func_ptr = (void (*)())m;

    if (argc > 1)
        strcpy(dest, argv[1]);

    (*func_ptr)();
    return 0;
}
