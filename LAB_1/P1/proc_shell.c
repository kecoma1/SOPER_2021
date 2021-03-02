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
#define BUF_SIZE 1024 /* Tamaño del buffer */

/*
    Estructura para pasarla al thread
*/
typedef struct {
    char linea[TAM_MAX];
    char *palabras[TAM_MAX];
    int num_palabras;
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

    /* Separando las palabras */
    mensaje->palabras[0] = strtok(mensaje->linea, " ");
    while ((mensaje->palabras[i] = (char *)strtok(NULL, " ")) != NULL && i < TAM_MAX) i++;
    mensaje->num_palabras = i;
	mensaje->palabras[0][strlen(mensaje->palabras[0])] = '\0';

    /* Quitando \n */
    for(int i = 0; i < strlen(mensaje->palabras[mensaje->num_palabras-1]); i++) 
        if(mensaje->palabras[mensaje->num_palabras-1][i] == '\n') 
            mensaje->palabras[mensaje->num_palabras-1][i] = '\0';
    return NULL;
}

int main() {
    pthread_t h1;
    input mensaje;
    int status = 0, pid = 0, flag = 0;
    int tuberia[2], nbytes = 0;
    char readbuffer[BUF_SIZE];
    FILE *pf = NULL;

    /* Inicializamos la string */
    memset(readbuffer, 0, BUF_SIZE);

    /* El proceso padre crea la tubería para log */
	if (pipe(tuberia) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

    /* Creamos el proceso log */
    pid = fork();
    if (pid == 0) {
        /* El hijo abre el archivo log.txt */
        pf = fopen("log.txt", "w");
        if (pf == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        /* El hijo cierra el extremo de escritura */
        close(tuberia[1]);
        do {
            /* Leemos la tubería */
			nbytes = read(tuberia[0], readbuffer, sizeof(readbuffer));
            if (nbytes == -1) {
                perror("read");
                while(fclose(pf) != 0 && errno != EINTR);
                exit(EXIT_FAILURE);
            }
			if(nbytes > 0) {
				fprintf(pf, "%.*s", (int) nbytes, readbuffer);
                if (fflush(pf) != 0) {
                    perror("fflush");
                    while(fclose(pf) != 0 && errno != EINTR);
                    exit(EXIT_FAILURE);
                }
			}
		} while(nbytes != 0);

        /* Cuando acabe liberamos recursos */
        while(fclose(pf) != 0 && errno != EINTR);
        exit(EXIT_FAILURE);
    } else if (pid > 0) close(tuberia[0]); /* El padre cierra el extremo de lectura */
    else {
        /* En caso de error */
        perror("fork");
        exit(EXIT_FAILURE);
    }

    /* Bucle principal */
    while(pid != 0) {
        printf(">>> ");
        /* Coge la cadena */
        if (fgets(mensaje.linea, TAM_MAX, stdin) == NULL) {
            /* Liberamos memoria, esperamos al proceso logger y salimos */
            printf("EOF detectado. ¡Adiós!\n");
            close(tuberia[1]);
            wait(&flag);
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
            /* El hijo ejecuta el comando */
            if (execvp(mensaje.palabras[0], mensaje.palabras)) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }   
        else {
            /* El padre espera al hijo e imprime el mensaje correspondiente */
            wait(&flag);

            /* Guardamos todo en una string concatenando las variables */
            memset(readbuffer, 0, BUF_SIZE); /* Inicializamos la string */
            strcat(readbuffer, "Comando: ");
            strcat(readbuffer, mensaje.linea);
            for (int i = 1; i < mensaje.num_palabras; i++){
                strcat(readbuffer, " ");
                strcat(readbuffer, mensaje.palabras[i]);
            }
            if (WIFEXITED(flag)) {
                /* Enviamos la string por la tubería escribiendo directamente en el descriptor */
                dprintf(tuberia[1], "%s, Exited with value %d\n", readbuffer, WEXITSTATUS(flag));
                printf("Exited with value %d\n", WEXITSTATUS(flag));
            } else if (WIFSIGNALED(flag)) {
                /* Enviamos la string por la tubería escribiendo directamente en el descriptor */
                dprintf(tuberia[1], "%s, Terminated by signal %d\n", readbuffer, WTERMSIG(flag));
                printf("Terminated by signal %d\n", WTERMSIG(flag));
            }
        }
    }
}