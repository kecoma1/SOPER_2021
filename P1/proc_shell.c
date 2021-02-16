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

#define TAM_MAX 100 /* Tamaño máximo de la cadena */

int main() {

    char *string = NULL;
    size_t tam_max = TAM_MAX;

    /* Bucle principal */
    while(1) {
        printf(">>> ");
        /* Coge la cadena */
        if (getline(&string, &tam_max, stdin) == -1) {
            /* Liberamos memoria y salimos */
            free(string);
            string = NULL;
            printf("EOF detectado. ¡Adiós!\n");
            return -1;
        }
    }
}