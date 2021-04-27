/**
 * @file trabajador.c
 * @author Archivo donde se codifica el comportamiento
 * del trbajador.
 * @brief 
 * @version 0.1
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "trabajador.h"

long int simple_hash(long int number) {
    long int result = (number * BIG_X + BIG_Y) % PRIME;
    return result;
}

void *work_thread(void *arg) {
    if (arg == NULL) {
        fprintf(stderr, "Error en work_thread. Null recibido.\n");
        return NULL;
    }

    worker_struct *indexes = (worker_struct *)arg;
    long int i = indexes->starting_index;

    /* Buscando el target */
    for (; i < indexes->ending_index; i++) {
        fprintf(stdout, "Trabajador %d. Searching... %6.2f%%\r", indexes->id, 100.0 * i / PRIME);
        if (indexes->target == simple_hash(i)) {
            fprintf(stdout, "\nSolution: %ld\n", i);
            exit(EXIT_SUCCESS);
        }
    }

    return NULL;
}
