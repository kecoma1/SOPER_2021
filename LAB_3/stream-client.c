#include "ui_struct.h"

int main (int argc, char *argv[]){

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
    if ((fd_shm = shm_open(SHM_NAME, O_RDONLY, 0)) == -1) {
        perror("CLIENT: shm_open");
        exit(EXIT_FAILURE);
    }

    /* Mapeamos la memoria compartida en una estructura */
    ui_shared = mmap(NULL, sizeof(ui_struct), PROT_READ, MAP_SHARED, fd_shm, 0);
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

    while(input != '\0'){
        input = ui_shared->buffer[ui_shared->get_pos % BUFFSIZE];
        printf("\nCLIENT: %c\n", input);
        sleep
        ui_shared->get_pos++;
    }

    ui_shared->get_pos++;
    /* Unmapping la memoria compartida */
    munmap(ui_shared, sizeof(ui_struct));
    fclose(pf);

    exit(EXIT_SUCCESS);
}