/**
 * @file proc_shell.c
 * @author Kevin de la Coba Malam
 *         Marcos Aaron Bernuy
 * @brief Archivo que genera una terminal sencilla
 * @version 1.0
 * @date 2021-02-16
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define TAM_MAX 100 /* Tamaño máximo de la cadena */

/*
    Estructura para pasarla al thread
*/
typedef struct {
    char *linea;
    char *palabras[TAM_MAX];
} input;

void *procesado_linea(void *arg) {

	input *mensaje = arg;
    char* line = NULL;
    int i = 1;
    
    /* Separamos las palabras */
    mensaje->palabras[0] = strtok(mensaje->linea, " ");

    /* Separando las palabras */
    while ((mensaje->palabras[i] = strtok(NULL, " ")) != NULL && i < TAM_MAX) i++;
	for (int n = 0; n < i; n++) printf("%s ", mensaje->palabras[n]);
    printf("\n");
    return NULL;
}

int main() {
    pthread_t h1;
    input mensaje;
    size_t tam_max = TAM_MAX;

    /* Inicializando variable */
    mensaje.linea = NULL;

    /* Bucle principal */
    while(1) {
        printf(">>> ");
        /* Coge la cadena */
        if (getline(&(mensaje.linea), &tam_max, stdin) == -1) {
            /* Liberamos memoria y salimos */
            if (&(mensaje.linea) != NULL) free(mensaje.linea);
            mensaje.linea = NULL;
            printf("EOF detectado. ¡Adiós!\n");
            return -1;
        }
        /* Creando el hilo */
        if (pthread_create(&h1, NULL, procesado_linea, (void*)&mensaje) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        /* Esperando al hilo */
        if (pthread_join(h1, NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }   
    }
}