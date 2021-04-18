#include "ui_struct.h"

int main(int argc, char *argv[]) {

    char *filename = argv[0];
    char input = '0';
    ui_struct *ui_shared = NULL;
    int fd_shm = 0;
    FILE *pf = NULL;
    struct timespec ts;

    if (argc < 1) {
        printf("\nEs necesario un fichero de entrada.\n");
        exit(EXIT_FAILURE);
    }

    ts.tv_sec = 2;

    /* Abrimos la memoria compartida */
    if ((fd_shm = shm_open(SHM_NAME, O_RDWR, 0)) == -1) {
        perror("CLIENT: shm_open");
        exit(EXIT_FAILURE);
    }

    /* Mapeamos la memoria compartida en una estructura */
    ui_shared = mmap(NULL, sizeof(ui_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (ui_shared == MAP_FAILED) {
        perror("SERVER: mmap");
        exit(EXIT_FAILURE);
    }

    pf = fopen(filename, "r");
    if (pf == NULL) {
        perror("SERVER: fopen");
        munmap(ui_shared, sizeof(ui_struct));
        exit(EXIT_FAILURE);
    }

    while(input != '\0'){
        /* Obteniendof el tiempo actual para el sem_timedwait */
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            exit(EXIT_FAILURE);

        ts.tv_sec += 2;

        if (sem_timedwait(&ui_shared->sem_empty, &ts) == -1 && errno == EINTR) {
            printf("Se desecha la operación.\n");
            continue;
        }
        if (sem_timedwait(&ui_shared->sem_mutex, &ts) == -1 && errno == EINTR) {
            printf("Se desecha la operación.\n");
            continue;
        }
        input = fgetc(pf);
        if (input == EOF)
            input = '\0';

        ui_shared->buffer[ui_shared->post_pos % BUFFSIZE] = input;
        ui_shared->post_pos++;

        sem_post(&ui_shared->sem_mutex);
        sem_post(&ui_shared->sem_fill);
    }


    /* Unmapping la memoria compartida */
    munmap(ui_shared, sizeof(ui_struct));
    fclose(pf);

    exit(EXIT_SUCCESS);
}