/**
 * @file stream-server.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se define el comportamiento del "server".
 * Aquí se lee un archivo y se escribe el contenido en la memoria 
 * compartida.
 * @version 1.0
 * @date 2021-04-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ui_struct.h"

int main(int argc, char *argv[]) {
    char *filename = argv[0];
    char input = '0';
    ui_struct *ui_shared = NULL;
    int fd_shm = 0;
    FILE *pf = NULL;
    struct timespec ts;
    Mensaje msg;

    if (argc < 1) {
        printf("\nSERVER: Es necesario un fichero de entrada.\n");
        exit(EXIT_FAILURE);
    }

    /* Abrimos la memoria compartida */
    if ((fd_shm = shm_open(SHM_NAME, O_RDWR, 0)) == -1) {
        perror("SERVER: shm_open");
        exit(EXIT_FAILURE);
    }

    /* Mapeamos la memoria compartida en una estructura */
    ui_shared = mmap(NULL, sizeof(ui_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (ui_shared == MAP_FAILED) {
        perror("SERVER: mmap");
        exit(EXIT_FAILURE);
    }

    /* Abrimos el archivo pasado como argumento */
    pf = fopen(filename, "r");
    if (pf == NULL) {
        perror("SERVER: fopen");
        munmap(ui_shared, sizeof(ui_struct));
        exit(EXIT_FAILURE);
    }

    /* Inicializamos la estructura de la cola de mensajes */
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(Mensaje)
    };

    /* Abrimos la cola de mensajes */
    mqd_t queue = mq_open(MQ_NAME_SERVER, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    if (queue == (mqd_t)-1) {
        perror("SERVER: mq_open");
        munmap(ui_shared, sizeof(ui_struct));
        fclose(pf);
        exit(EXIT_FAILURE);
    }

    /* Bucle principal de lectura */
    while(input != '\0'){
        /* Recibimos la instruccion */
        if (mq_receive(queue, (char *)&msg, sizeof(msg), NULL) == -1){
            perror("SERVER: mq_receive");
            munmap(ui_shared, sizeof(ui_struct));
            fclose(pf);
            mq_close(queue);
            exit(EXIT_FAILURE);
        }

        /* Salimos del bucle para finalizar la ejecución */
        if(strncmp(msg.message, "exit", 4) == 0){
            break;
        }

        /* Obteniendo el tiempo actual para el sem_timedwait */
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("SERVER: clock_gettime");
            munmap(ui_shared, sizeof(ui_struct));
            fclose(pf);
            mq_close(queue);
            exit(EXIT_FAILURE);
        }
        /* 2 segundos desde el momento actual */
        ts.tv_sec += 2;

        if (sem_timedwait(&ui_shared->sem_empty, &ts) == -1 && errno == EINTR) {
            printf("SERVER: Se desecha la operación.\n");
            continue;
        }
        if (sem_timedwait(&ui_shared->sem_mutex, &ts) == -1 && errno == EINTR) {
            printf("SERVER: Se desecha la operación.\n");
            continue;
        }
        input = fgetc(pf);

        /* Final del archivo */
        if (input == EOF) {
            printf("SERVER: No hay más que escribir. - ");
            input = '\0';
        }

        ui_shared->buffer[ui_shared->post_pos % BUFFSIZE] = input;
        ui_shared->post_pos++;

        sem_post(&ui_shared->sem_mutex);
        sem_post(&ui_shared->sem_fill);
    }

    printf("SERVER: Ejecución finalizada.\n>>> ");

    /* Unmapping la memoria compartida */
    munmap(ui_shared, sizeof(ui_struct));

    /* Cerrando el archivo */
    fclose(pf);

    /* Cerrando la cola de mensajes */
    mq_close(queue);

    exit(EXIT_SUCCESS);
}