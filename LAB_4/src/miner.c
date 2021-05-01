/**
 * @file miner.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se define la implementación de los
 * mineros
 * @version 0.2 - Implementación bloques.
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "miner.h"

int main(int argc, char *argv[]) {
    extern int solution_find;
    long int target = 0;
    int num_workers = 0, i = 0, err = 0, rounds = 0, infinite = 0;
    worker_struct *threads_info = NULL;
    pthread_t threads[MAX_WORKERS];
    Block *last_block = NULL;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NUMERO TRABAJADORES> <RONDAS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Generamos un target aleatorio entre 1 - 1.000.000 */
    srand(time(NULL));

    /* Establecemos un target y el número de trabajadores */
    num_workers = atoi(argv[1]);
    rounds = atol(argv[2]);

    /* En caso de que el número de rondas sea infinito */
    if (rounds <= 0) {
        infinite = 1;
    }

    if (num_workers > MAX_WORKERS) {
        fprintf(stderr, "Número incorrecto de trabajadores. Defina un número entre [1-10] (ambos incluidos).\n");
        exit(EXIT_FAILURE);
    }

    /* Reservamos memoria para la estructura usada por los threads  */
    threads_info = (worker_struct*)malloc(num_workers*(sizeof(worker_struct)));
    if (threads_info == NULL) {
        perror("Error reservando memoria para la estructura de los trabajadores. malloc");
        exit(EXIT_FAILURE);
    }

    /* Ejecutando las rondas correspondientes */
    for (int n = 0; n < rounds || infinite == 1; n++) {

        /* Creamos el bloque */
        Block *block = block_ini();
        if (block == NULL) {
            fprintf(stderr, "Error creando el bloque. block_ini.\n");
            free(threads_info);
            exit(EXIT_FAILURE);
        }

        if (block_set(last_block, block) == -1) {
            fprintf(stderr, "Error inicializando el bloque. block_set.\n");
            free(threads_info);
            exit(EXIT_FAILURE);
        }

        /* Creando threads */
        for (i = 0; i < num_workers; i++) {
            /* Inicializamos las estructuras para los threads */
            threads_info[i].target = block->target;
            threads_info[i].starting_index = i*(PRIME/num_workers);
            threads_info[i].ending_index = (i+1)*(PRIME/num_workers);

            err = pthread_create(&threads[i], NULL, work_thread, (void *)&threads_info[i]);
            if (err != 0) {
                perror("Error creando threads. pthread_create");
                free(threads_info);
                exit(EXIT_FAILURE);
            }
        }

        /* Terminando la ejecución de los threads */
        for (i = 0; i < num_workers; i++) {
            err = pthread_join(threads[i], NULL);
            if (err != 0) {
                perror("pthread_join");
                free(threads_info);
                exit(EXIT_FAILURE);
            }
            
            /* Establecemos la solución */
            if (threads_info[i].solution != -1)
                block->solution  = threads_info[i].solution;
        }

        print_blocks(block, 20);
        last_block = block;
        solution_find = 0;
    }

    /* Liberamos recursos */
    free(threads_info);

    exit(EXIT_FAILURE);
}
