#include "ui_struct.h"

int main(int argc, char *argv[]) {

    char *filename = argv[0];
    char input = '0';
    ui_struct *ui_shared = NULL;
    int fd_shm = 0;
    FILE *pf = NULL;

    if (argc < 1) {
        printf("\nEs necesario un fichero de entrada.\n");
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

    pf = fopen(filename, "r");
    if (pf == NULL) {
        perror("SERVER: fopen");
        munmap(ui_shared, sizeof(ui_struct));
        exit(EXIT_FAILURE);
    }

    while((input = fgetc(pf)) != EOF){
        ui_shared->buffer[ui_shared->post_pos % BUFFSIZE] = input;
        printf("\nSERVER: %c\n", input);
        ui_shared->post_pos++;
        sleep(1);
    }

    /* Escribiendo el final */
    ui_shared->post_pos++;
    ui_shared->buffer[ui_shared->post_pos] = '\0';

    /* Unmapping la memoria compartida */
    munmap(ui_shared, sizeof(ui_struct));
    fclose(pf);

    exit(EXIT_SUCCESS);
}