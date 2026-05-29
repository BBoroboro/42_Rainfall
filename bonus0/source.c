#include <stdio.h>
#include <string.h>
#include <unistd.h>

void p(char *dest, const char *message) {
    char buffer[4096];

    puts(message);                      // Affiche le message
    read(0, buffer, 0x1000);            // Lit jusqu’à 4096 octets depuis stdin

    char *newline = strchr(buffer, '\n');
    if (newline)
        *newline = '\0';                // Remplace le '\n' par '\0'

    strncpy(dest, buffer, 20);          // Copie max 20 chars
    dest[19] = '\0';                    // Sécurisation
}

void pp(char *output) {
    char first[20];
    char second[20];

    // Affiche DAT_080486a0 deux fois (probablement un prompt)
    p(first,  (char *)0x080486a0);
    p(second, (char *)0x080486a0);

    strcpy(output, first);

    // Trouver la fin de output
    size_t len = strlen(output);

    // Remplace le \0 final par un espace
    output[len] = ' ';
    output[len + 1] = '\0';

    // Ajoute second à la suite
    strcat(output, second);
}

int main(void) {
    char buffer[54];

    pp(buffer);
    puts(buffer);

    return 0;
}
