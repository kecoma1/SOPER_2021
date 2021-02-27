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
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#define TAM_MAX 100 /* Tamaño máximo de la cadena */

/*
    Estructura para pasarla al thread
*/
typedef struct {
    char linea[TAM_MAX];
    char *palabras[TAM_MAX];
} input;

/**
 * @brief Función para procesar lineas. Es ejecutada por un hilo hijo
 * 
 * @param arg Estructura que contiene el mensaje y el array de char* donde guardar cada palabra
 * @return void* NULL
 */
void *procesado_linea(void *arg) {
    input *mensaje = arg;
    char* line = NULL, *str = NULL;
    int i = 1;
    
    /* Separamos las palabras */
    mensaje->palabras[0] = strtok(mensaje->linea, " ");

    /* Separando las palabras */
    while ((mensaje->palabras[i] = (char *)strtok(NULL, " ")) != NULL && i < TAM_MAX) {
        
        i++;
    } 
    int n = i;
	mensaje->palabras[0][strlen(mensaje->palabras[0])] = '\0';
    /* Quitando \n */
    for(int i = 0; i < strlen(mensaje->palabras[n-1]); i++) if(mensaje->palabras[n-1][i] == '\n') mensaje->palabras[n-1][i] = '\0';
    return NULL;
}

int main() {
    pthread_t h1;
    input mensaje;
    int status = 0, pid = 0, flag = 0;

    /* Bucle principal */
    while(1) {
        printf(">>> ");
        /* Coge la cadena */
        if (fgets(mensaje.linea, TAM_MAX, stdin) == NULL) {
            /* Liberamos memoria y salimos */
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

        /* Hacemos un fork para obtener proceso padre e hijo */
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            if (execvp(mensaje.palabras[0], mensaje.palabras)) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }   
        else {
            wait(&flag);
            if (WIFEXITED(flag)) { 
                printf("Exited with value %d\n", WEXITSTATUS(flag));
            } else if (WIFSIGNALED(flag)) {
                WTERMSIG(flag);
                printf("Terminated by signal %d\n", WTERMSIG(flag));
            }
        }
    }
}