#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int *obj1 = malloc(8);
    obj1[0] = 1;
    obj1[1] = (int)malloc(8);

    int *obj2 = malloc(8);
    obj2[0] = 2;
    obj2[1] = (int)malloc(8);

    strcpy((char*)obj1[1], argv[1]);
    strcpy((char*)obj2[1], argv[2]);

    FILE *f = fopen("/home/user/level8/.pass", "r");
    char c[0x44];
    if (f != NULL) {
        fgets(c, sizeof(c), f);
        fclose(f);
    }

    puts("~~");
    return 0;
}