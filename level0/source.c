#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        return 1;
    }

    int value = atoi(argv[1]);

    if (value == 0x1A7) {  // 423 en décimal
        char *shell = strdup("/bin/sh");
        uid_t uid = geteuid();
        gid_t gid = getegid();

        setresgid(gid, gid, gid);
        setresuid(uid, uid, uid);

        char *args[] = { shell, NULL };
        execv("/bin/sh", args);

        perror("execv");
        free(shell);
    } 
    else {
        fprintf(stderr, "No!\n");
    }
    return 0;
}
