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

#include "ui_struct.h"

int main(int argc, char *argv[]) {
    pid_t pid_ui = 0, pid_server = 0, pid_client = 0;
    char input[INPUTMAXSIZE] = "\0";
    int fd_shm = 0;
    ui_struct *ui_shared = NULL;

    if (argc < 3) {
        printf("Se deben incluir un fichero de entrada y otro de salida.\n./stream-ui <salida> <entrada>\n");
        return -1;
    }

    pid_ui = getppid();

    /* Abrimos la memoria compartida */
    if ((fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR)) == -1) {
        perror("shm_open");
        shm_unlink(SHM_NAME);
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
    
    /* Inicializando variables */
    strcpy(ui_shared->buffer, "00000");
    ui_shared->post_pos = 0;
    ui_shared->get_pos = 0;

    /* Inicializando semáforos */
    if (sem_init(&ui_shared->sem_fill, 1, 0) == -1) {
        perror("sem_init");
        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    if (sem_init(&ui_shared->sem_empty, 1, 1) == -1) {
        perror("sem_init");
        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    if (sem_init(&ui_shared->sem_mutex, 1, 1) == -1) {
        perror("sem_init");
        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }


    /* Creamos el proceso server */
    pid_server = fork();
    if (pid_server == -1) {
        perror("fork");
        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    /* Ejecución del proceso server */
    if (pid_server == 0) {
        if (execl("./stream-server", argv[2], (char*) NULL) == -1) {
            perror("execl");
            munmap(ui_shared, sizeof(ui_struct));
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
    } 

    if (pid_ui == getppid()) {
        /* Creamos el proceso client */
        pid_client = fork();
        if (pid_client == -1) {
            perror("fork");
            /* Matamos al proceso server */
            kill(pid_server, SIGKILL);
            munmap(ui_shared, sizeof(ui_struct));
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
    }
    /* Ejecución del proceso client */
    if (pid_client == 0) {
        if (execl("./stream-client", argv[1], (char*) NULL) == -1) {
            perror("execl");
            munmap(ui_shared, sizeof(ui_struct));
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
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

    /* Destruyendo semaforos */
    sem_destroy(&ui_shared->sem_fill);
    sem_destroy(&ui_shared->sem_empty);
    sem_destroy(&ui_shared->sem_mutex);

    return 0;
}