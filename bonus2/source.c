#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int language = 0;

void greetuser(void) {
    char greeting[100] = "";
    char local_4c[8];
    char local_44[64];

    if (language == 1) {
        strncpy(local_4c, "Hyv\xC3\xA4", 4);
        strncpy(local_44, "päivää ", sizeof(local_44));
    } else if (language == 2) {
        strncpy(local_4c, "Goed", 4);
        strncat(local_4c, "emid", 4);
        strncpy(local_44, "dag! ", sizeof(local_44));
    } else {
        strncpy(local_4c, "Hell", 4);
        strncat(local_4c, "o ", 2);
        strncpy(local_44, "world!", sizeof(local_44));
    }

    strcat(greeting, local_4c);
    strcat(greeting, local_44);

    puts(greeting);
}

int main(int argc, char *argv[]) {
    if (argc == 3) {
        char local_60[40] = {0};
        char acStack_38[36] = {0};

        strncpy(local_60, argv[1], sizeof(local_60) - 1);
        strncpy(acStack_38, argv[2], sizeof(acStack_38) - 1);

        char *lang_env = getenv("LANG");
        if (lang_env != NULL) {
            if (strncmp(lang_env, "fi", 2) == 0) {
                language = 1;
            } else if (strncmp(lang_env, "nl", 2) == 0) {
                language = 2;
            }
        }

        greetuser();
        return 0;
    } else {
        printf("Usage: %s <arg1> <arg2>\n", argv[0]);
        return 1;
    }
}
