#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    FILE *f;
    char pass1[0x42];  // 66
    char pass2[0x41];  // 65
    int index;

    memset(pass1, 0, sizeof(pass1));
    memset(pass2, 0, sizeof(pass2));

    f = fopen("/home/user/end/.pass", "r");
    if (!f || argc != 2) {
        return -1;
    }

    fread(pass1, 1, 0x42, f);

    index = atoi(argv[1]);
    pass1[index] = '\0';

    fread(pass2, 1, 0x41, f);

    fclose(f);

    if (strcmp(pass1, argv[1]) == 0) {
        execl("/bin/sh", "sh", NULL);
    } else {
        puts(pass2);
    }

    return 0;
}
