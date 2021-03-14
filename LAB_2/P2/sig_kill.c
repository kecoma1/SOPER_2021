#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int sig;
    pid_t pid;
    pid_t pid_fork;
    char *array[3] = {"kill", "-SIGSTOP", "4321"};
    char *p[3] = {"rm", "-rf", "p"};

    if (argc != 3) {
        fprintf(stderr, "Uso: %s -<signal> <pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sig = atoi(argv[1] + 1);
    pid = (pid_t)atoi(argv[2]);

    /* Rellenar Código */

    /* Una de las pruebas que estaba haciendo */
    /* Por algun motivo funciona con p pero no con array */
    if (execlp(array[0], array[1], array[2], (char*)NULL)) {
        perror("execl");
        exit(EXIT_FAILURE);
    }

    /*
    argv[3] = NULL;
    if (execvp("kill", argv)) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    */
    exit(EXIT_SUCCESS);
}
