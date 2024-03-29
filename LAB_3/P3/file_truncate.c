#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define SHM_NAME "/shm_example"
#define MESSAGE "Test message"


int main(int argc, char *argv[]) {
    int fd;
    struct stat statbuf;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <FILE>\n", argv[0]);
    }

    fd = open(argv[1], O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    dprintf(fd, "%s", MESSAGE);
    /* Get size of the file. */
    struct stat fileStat;
    fstat(fd, &fileStat);
    printf("Tamaño del fichero %ld\n", fileStat.st_size);

    /* Truncate the file to size 5. */
    ftruncate(fd, 5*sizeof(char));

    fstat(fd, &fileStat);
    printf("Tamaño del fichero %ld\n", fileStat.st_size);

    close(fd);
    exit(EXIT_SUCCESS);
}
