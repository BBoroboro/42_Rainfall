#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *auth = NULL;
char *service = NULL;

int main(void) {
    char buf[128];

    while (1) {
        printf("%p, %p \n", auth, service);

        if (!fgets(buf, sizeof(buf), stdin))
            return 0;

        buf[strcspn(buf, "\n")] = 0; // enlever le '\n'

        /* --- Commande "auth" --- */
        if (strncmp(buf, "auth", 4) == 0) {
            auth = malloc(4);     // exactement comme dans l'ASM
            if (!auth) return 1;

            memset(auth, 0, 4);

            // Dans le binaire, la copie lit dans local_8b (2 bytes)
            // On imite donc : copie une chaîne vide ou très courte
            char small[2] = {0};
            strcpy(auth, small);  // strcpy volontaire (comme dans l’original)
        }

        /* --- Commande "reset" --- */
        else if (strncmp(buf, "reset", 5) == 0) {
            free(auth);
            auth = NULL;
        }

        /* --- Commande "service" --- */
        else if (strncmp(buf, "service", 7) == 0) {
            char tmp[128] = {0};
            // Dans l’original acStack_89 est non rempli → on simule
            strcpy(tmp, "");  
            service = strdup(tmp);
        }

        /* --- Commande "login" --- */
        else if (strncmp(buf, "login", 5) == 0) {
            // Le binaire teste auth + 0x20 → hors limites.
            // On simule le comportement :
            if (!auth || *(int *)(auth + 0x20) == 0) {
                printf("Password:\n");
            } else {
                system("/bin/sh");
            }
        }
    }

    return 0;
}