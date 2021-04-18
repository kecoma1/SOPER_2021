#include "ui_struct.h"

int main(int argc, char *argv[]){

    char *filename = argv[0];
    char input = '0';
    FILE *pf = NULL;
    ui_struct *ui_shared = NULL;
    int fd_shm = 0;

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

    /* Hacemos un  bucle para escribir en el fichero */
    while(input != '\0') {
        sem_wait(&ui_shared->sem_fill);
        sem_wait(&ui_shared->sem_mutex);

        input = ui_shared->buffer[ui_shared->get_pos % BUFFSIZE];
        if (input == '\0') break;
        fwrite(&input, 1, 1, pf);
        ui_shared->get_pos++;

        sem_post(&ui_shared->sem_mutex);
        sem_post(&ui_shared->sem_empty);
    }

    ui_shared->get_pos++;
    printf("\nCLIENT: SALGO\n");
    /* Unmapping la memoria compartida */
    munmap(ui_shared, sizeof(ui_struct));
    fclose(pf);

    exit(EXIT_SUCCESS);
}