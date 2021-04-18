#include "ui_struct.h"

int main(int argc, char *argv[]){

    char *filename = argv[0];
    char input = '0';
    FILE *pf = NULL;
    ui_struct *ui_shared = NULL;
    int fd_shm = 0;
    struct timespec ts;
    Mensaje msg;

    if (argc < 1){
        printf("\nNo se ha recibido fichero de salida.\n");
        exit(EXIT_FAILURE);
    }

    /* Abrimos la memoria compartida */
    if ((fd_shm = shm_open(SHM_NAME, O_RDWR, 0)) == -1) {
        perror("CLIENT: shm_open");
        exit(EXIT_FAILURE);
    }

    /* Mapeamos la memoria compartida en una estructura */
    ui_shared = mmap(NULL, sizeof(ui_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (ui_shared == MAP_FAILED) {
        perror("CLIENT: mmap");
        exit(EXIT_FAILURE);
    }

    /* Abrimos el archivo pasado como argumento */
    pf = fopen(filename, "w");
    if(pf == NULL){
        perror("CLIENT: fopen");
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
    mqd_t queue = mq_open(MQ_NAME_CLIENT, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
    if (queue == (mqd_t)-1) {
        perror("mq_open");
        munmap(ui_shared, sizeof(ui_struct));
        exit(EXIT_FAILURE);
    }

    /* Hacemos un  bucle para escribir en el fichero */
    while(input != '\0') {
        /* Recibimos la instruccion mandada */
        if (mq_receive(queue, (char *)&msg, sizeof(msg), NULL) == -1){
            perror("mq_receive");
            munmap(ui_shared, sizeof(ui_struct));
            fclose(pf);
            mq_close(queue);
            exit(EXIT_FAILURE);
        }

        /* Salimos del bucle para finalizar la ejecución */
        if(strncmp(msg.message, "exit", 4)){
            break;
        }

        /* Obteniendo el tiempo actual para el sem_timedwait */
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            perror("clock_gettime");
            munmap(ui_shared, sizeof(ui_struct));
            fclose(pf);
            mq_close(queue);
            exit(EXIT_FAILURE);
        }

        ts.tv_sec += 2;

        if (sem_timedwait(&ui_shared->sem_fill, &ts) == -1 && errno == EINTR) {
            printf("Se desecha la operación.\n");
            continue;
        }
        if (sem_timedwait(&ui_shared->sem_mutex, &ts) == -1 && errno == EINTR) {
            printf("Se desecha la operación.\n");
            continue;
        }

        input = ui_shared->buffer[ui_shared->get_pos % BUFFSIZE];
        if (input == '\0') {
            sem_post(&ui_shared->sem_mutex);
            sem_post(&ui_shared->sem_empty);
            break;
        }
        fwrite(&input, 1, 1, pf);
        ui_shared->get_pos++;

        sem_post(&ui_shared->sem_mutex);
        sem_post(&ui_shared->sem_empty);
    }

    /* Unmapping la memoria compartida */
    munmap(ui_shared, sizeof(ui_struct));
    fclose(pf);

    /* Cerrando la cola de mensajes */
    mq_close(queue);

    exit(EXIT_SUCCESS);
}