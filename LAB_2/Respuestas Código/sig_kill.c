#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int sig;
    pid_t pid;

    if (argc != 3) {
        fprintf(stderr, "Uso: %s -<signal> <pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sig = atoi(argv[1] + 1);
    pid = (pid_t)atoi(argv[2]);

    /* Rellenar Código */
    kill(pid, sig);

    exit(EXIT_SUCCESS);
}
