/**
 * @file trabajador.c
 * @author Archivo donde se codifica el comportamiento
 * del trbajador.
 * @brief Archivo donde se codifican las funciones usadas
 * para que los trabajadores trabajen.
 * @version 0.1
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "trabajador.h"

int solution_find = 0;

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
        if (solution_find == 1) break;

        printf("solution: %d, i = %ld\n", solution_find, i);
        solution_find++;
        sleep(1);
        if (i == 5 || indexes->target == simple_hash(i)) {
            fprintf(stdout, "\nSolution: %ld\n", i);
            indexes->solution =  i;
            solution_find = 1;
            return NULL;
        }
    }

    return NULL;
}
