#include <errno.h>
#include <stdio.h>

void main() {
    int x = 0;

    if (( fopen("nombre", "r") == 0 )) {
        // Guardamos el valor antes de imprimirlo
        x = errno;
        fprintf( stderr, "errno: %d\n", errno);

        // Reasignamos el valor de errno
        errno = x;
        perror("fopen")
    }
}