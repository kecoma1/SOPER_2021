#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void *slow_printf(void *arg)
{
    const char *msg = arg;
    size_t i;
    for (i = 0; i < strlen(msg); i++)
    {
        printf(" %c ", msg[i]);
        fflush(stdout);
        sleep(1);
    }
    return NULL;
}
int main(int argc, char *argv[])
{
    pthread_t h1;
    pthread_t h2;
    char *hola = " Hola ";
    char *mundo = " Mundo ";
    int error;
    error = pthread_create(&h1, NULL, slow_printf, hola);
    pthread_detach(h1);
    if (error != 0)
    {
        fprintf(stderr, " pthread_create : %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }
    error = pthread_create(&h2, NULL, slow_printf, mundo);
    pthread_detach(h2);
    if (error != 0)
    {
        fprintf(stderr, " pthread_create : %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }
    //error = pthread_join(h1, NULL);
    if (error != 0)
    {
        fprintf(stderr, " pthread_join : %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }
    //error = pthread_join(h2, NULL);
    if (error != 0)
    {
        fprintf(stderr, " pthread_join : %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }
    printf("El programa %s termino correctamente \n", argv[0]);
    fflush(stdout);
    exit(EXIT_SUCCESS);
}
