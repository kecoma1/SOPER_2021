/**
 * @file miner.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se define la implementación de los
 * mineros
 * @version 0.1 - Minero paralelo.
 *          0.2 - Implementación bloques.
 *          0.3 - Memoria compartida bloques.
 *          0.4 - Red de mineros.
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "miner.h"

extern int solution_find;
char sig_int_recibida = 0;

/**
 * @brief Manejador de la señal SIGALARM
 * Cuando se recibe la señal alarm, el comportamiento 
 * es el mismo al de la señal sigint, por lo que modificamos
 * el valor de singint
 * @param sig Señal
 */
void manejador_SIGINT(int sig) {
    printf("Minero %d, abandona la red :-(.\n", (int)getpid());
    
    /* Como el proceso se va a cerrar, hacemos que los trabajadores
    acaben su ejecución cuanto antes */
    solution_find = 1;

    sig_int_recibida = 1;
}

int main(int argc, char *argv[]) {
    long int target = 0;
    int num_workers = 0, i = 0, err = 0, rounds = 0, infinite = 0;

    pthread_t threads[MAX_WORKERS];

    worker_struct *threads_info = NULL;
    Block *last_block = NULL, *block = NULL;
    NetData *net = NULL;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NUMERO TRABAJADORES> <RONDAS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /* Inicializamos una máscara para ignorar SIGINT 
    hasta que todo este inicializado */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* Inicializamos sigaction para la señal SIGINT */
    struct sigaction act_SIGINT;
    act_SIGINT.sa_handler = manejador_SIGINT;
    sigemptyset(&(act_SIGINT.sa_mask));
    /* Ignoramos la señal SIGUSR2 (ya que el proceso se estará cerrando) */
    sigaddset(&(act_SIGINT.sa_mask), SIGUSR2);
    act_SIGINT.sa_flags = 0;
    if(sigaction(SIGINT, &act_SIGINT, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Creamos/accedemos a la red */
    net = create_net();
    if (net == NULL) {
        fprintf(stderr, "Error al crear/acceder a la red de mineros.");
        exit(EXIT_FAILURE);
    }

    /* Generamos un target aleatorio entre 1 - 1.000.000 */
    srand(time(NULL));

    /* Establecemos un target y el número de trabajadores */
    num_workers = atoi(argv[1]);
    rounds = atol(argv[2]);

    /* En caso de que el número de rondas sea infinito */
    if (rounds <= 0) infinite = 1;

    /* Creamos/linkeamos la memoria compartida */
    shared_block_info *sbi = create_shared_block_info();
    if (sbi == NULL) {
        fprintf(stderr, "Error al crear/linkear la memoria compartida.\n");
        close_net(net);
        exit(EXIT_FAILURE);
    }

    if (num_workers > MAX_WORKERS) {
        fprintf(stderr, "Número incorrecto de trabajadores. Defina un número entre [1-10] (ambos incluidos).\n");
        close_net(net);
        close_shared_block_info(sbi);
        exit(EXIT_FAILURE);
    }

    /* Reservamos memoria para la estructura usada por los threads  */
    threads_info = (worker_struct*)malloc(num_workers*(sizeof(worker_struct)));
    if (threads_info == NULL) {
        perror("Error reservando memoria para la estructura de los trabajadores. malloc");
        close_net(net);
        close_shared_block_info(sbi);
        exit(EXIT_FAILURE);
    }

    /* Como ya está todo inicializado volvemos a permitir la señal SIGINT */
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* Ejecutando las rondas correspondientes */
    for (int n = 0; n < rounds || infinite == 1; n++) {

        /* Creamos el bloque */
        block = block_ini();
        if (block == NULL) {
            fprintf(stderr, "Error creando el bloque. block_ini.\n");
            free(threads_info);
            close_net(net);
            close_shared_block_info(sbi);
            exit(EXIT_FAILURE);
        }

        if (block_set(last_block, block) == -1) {
            fprintf(stderr, "Error inicializando el bloque. block_set.\n");
            free(threads_info);
            close_net(net);
            close_shared_block_info(sbi);
            exit(EXIT_FAILURE);
        }

        /* Controlamos el valor de la memoria concurrente el semáforo mutex */
        while(sem_wait(&sbi->mutex) == -1) {
            if (errno != EINTR) {
                perror("sem_wait");
                free(threads_info);
                close_net(net);
                close_shared_block_info(sbi);
                exit(EXIT_FAILURE);
            }
        }
        if (sbi->target != block->target) block->target = sbi->target;
        sem_post(&sbi->mutex);

        /* Creando threads */
        for (i = 0; i < num_workers; i++) {
            /* Inicializamos las estructuras para los threads */
            threads_info[i].target = block->target;
            threads_info[i].starting_index = i*(PRIME/num_workers);
            threads_info[i].ending_index = (i+1)*(PRIME/num_workers);
            threads_info[i].solution = -1;
            

            /* Creando los threads */
            err = pthread_create(&threads[i], NULL, work_thread, (void *)&threads_info[i]);
            if (err != 0) {
                perror("Error creando threads. pthread_create");
                free(threads_info);
                close_net(net);
                close_shared_block_info(sbi);
                exit(EXIT_FAILURE);
            }
        }

        /* Terminando la ejecución de los threads */
        for (i = 0; i < num_workers; i++) {

            err = pthread_join(threads[i], NULL);
            if (err != 0) {
                perror("pthread_join");
                free(threads_info);
                close_net(net);
                close_shared_block_info(sbi);
                exit(EXIT_FAILURE);
            }
            
            /* Establecemos la solución */
            if (threads_info[i].solution != -1) {
                block->solution  = threads_info[i].solution;

                /* Actualizamos el valor del target tras haber encontrado la solución */
                while(sem_wait(&sbi->mutex) == -1) {
                    if (errno != EINTR) {
                        perror("sem_wait");
                        free(threads_info);
                        close_net(net);
                        close_shared_block_info(sbi);
                        exit(EXIT_FAILURE);
                    }
                }
                sbi->target = block->solution;
                sem_post(&sbi->mutex);
            }
        }

        /* No debería ejecutarse nunca */
        if (block->solution == -1) {
            printf("No se ha encontrado la solución.\n");
            break;
        }

        /* Abandonamos el bucle principal si se ha recibido SIGINT */
        if (sig_int_recibida == 1) break;

        last_block = block;
        solution_find = 0;
    }

    /* Liberamos recursos */
    close_net(net);
    close_shared_block_info(sbi);
    block_destroy_blockchain(block);
    free(threads_info);

    exit(EXIT_FAILURE);
}
