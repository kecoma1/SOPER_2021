/**
 * @file stream-ui.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se define el comportamiento de la UI del streaming.
 * Aquí se crean los procesos server y client, y se manejan las instrucciones
 * del usuario.
 * @version 1.0
 * @date 2021-04-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <fcntl.h>

#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include <signal.h>

#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>

#define INPUTMAXSIZE 128
#define BUFFSIZE 5
#define SHM_NAME "/shm_example"


typedef struct {
    char buffer[BUFFSIZE];
    int post_pos;
    int get_pos;
} ui_struct;


int main() {
    pid_t pid_ui = 0, pid_server = 0, pid_client = 0;
    char input[INPUTMAXSIZE] = "\0";
    int fd_shm = 0;
    ui_struct *ui_shared;

    pid_ui = getppid();

    /* Abrimos la memoria compartida */
    if ((fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR)) == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    /* Reajustamos el tamaño de la memoria compartida */
    if (ftruncate(fd_shm, sizeof(ui_struct)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Mapeamos la memoria compartida en una estructura */
    ui_shared = mmap(NULL, sizeof(ui_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (ui_shared == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    
    /* Creamos el proceso server */
    pid_server = fork();
    if (pid_server == -1) {
        perror("fork");
        return -1;
    }
    /* Ejecución del proceso server */
    if (pid_server == 0) {
        return -1;
    } 

    if (pid_ui == getppid()) {
        /* Creamos el proceso client */
        pid_client = fork();
        if (pid_client == -1) {
            perror("fork");
            /* Matamos al proceso server */
            kill(pid_server, SIGKILL);
            return -1;
        }
    }
    /* Ejecución del proceso client */
    if (pid_client == 0) {
        return -1;
    }

    /* Recogemos los comandos */
    while(strcmp("exit", input) != 0) {
        printf(">>> ");
        scanf("%s", input);
    }

    /* Esperamos a los procesos */
    waitpid(pid_server, NULL, 0);
    waitpid(pid_client, NULL, 0);
    
    /* Liberando recursos */
    munmap(ui_shared, sizeof(ui_struct));
    shm_unlink(SHM_NAME);

    return 0;
}