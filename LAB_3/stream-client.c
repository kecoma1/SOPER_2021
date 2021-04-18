#include "ui_struct.h"

int main (int argc, char *argv[]){

    char *filename = argv[0];
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
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    printf("\nCLIENT: %d %d\n", ui_shared->post_pos, ui_shared->get_pos);
    /* Unmapping la memoria compartida */
    munmap(ui_shared, sizeof(ui_struct));

    exit(EXIT_SUCCESS);
}