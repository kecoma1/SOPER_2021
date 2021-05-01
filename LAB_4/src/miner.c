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

void print_blocks(Block *plast_block, int num_wallets) {
    Block *block = NULL;
    int i, j;

    for(i = 0, block = plast_block; block != NULL; block = block->prev, i++) {
        printf("Block number: %d; Target: %ld;    Solution: %ld\n", block->id, block->target, block->solution);
        for(j = 0; j < num_wallets; j++) {
            printf("%d: %d;         ", j, block->wallets[j]);
        }
        printf("\n\n\n");
    }
    printf("A total of %d blocks were printed\n", i);
}

int main(int argc, char *argv[]) {
    long int target = 0;
    int num_workers = 0, i = 0, err = 0, rounds = 0;
    worker_struct *threads_info = NULL;
    pthread_t threads[MAX_WORKERS];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NUMERO TRABAJADORES> <RONDAS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Generamos un target aleatorio entre 1 - 1.000.000 */
    srand(time(NULL));
    target = rand () % (1000000-1+1) + 1;
    printf("Primer target: %d\n", target);

    /* Establecemos un target y el número de trabajadores */
    num_workers = atoi(argv[1]);
    rounds = atol(argv[2]);

    if (num_workers > MAX_WORKERS || num_workers <= 0) {
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
    for (int n = 0; n < rounds; n++) {

        /* Creamos el bloque */

        /* Creando threads */
        for (i = 0; i < num_workers; i++) {
            /* Inicializamos las estructuras para los threads */
            threads_info[i].target = target;
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
        }
    }

    /* Liberamos recursos */
    free(threads_info);

    exit(EXIT_FAILURE);
}
