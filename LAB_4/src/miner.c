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
short sig_int_recibida = 0;
short sig_usr2_recibida = 0;


/**
 * @brief Manejador de la señal SIGINT
 * 
 * @param sig Señal
 */
void manejador_SIGINT(int sig) {
    printf("Minero %d, abandona la red :-(.\n", (int)getpid());
    
    /* Como el proceso se va a cerrar, hacemos que los trabajadores
    acaben su ejecución cuanto antes */
    solution_find = 1;

    sig_int_recibida = 1;
}

/**
 * @brief Manejador de la señal SIGUSR2
 * 
 * @param sig Señal
 */
void manejador_SIGUSR2(int sig) {
    
    /* Para efectuar la votación hacemos que los threads 
    acaben cuanto antes */
    solution_find = 1;

    sig_usr2_recibida = 1;
}

/**
 * @brief Manejador de la señal SIGUSR1. Vacío.
 * 
 * @param sig Señal
 */
void manejador_SIGUSR1(int sig) {}

int main(int argc, char *argv[]) {
    long int target = 0;
    int num_workers = 0, i = 0, err = 0, rounds = 0, infinite = 0;
    int index = 0;

    pthread_t threads[MAX_WORKERS];

    worker_struct *threads_info = NULL;
    Block *last_block = NULL, *block = NULL;
    pid_t pid = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NUMERO TRABAJADORES> <RONDAS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    pid = getpid();

    /* Inicializamos una máscara para ignorar SIGINT 
    /* Las unicas señales que no igoramos on */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* Inicializamos las estructuras sigaction */
    struct sigaction act_SIGINT, act_SIGUSR2, act_SIGUSR1;
    act_SIGINT.sa_handler = manejador_SIGINT;
    act_SIGUSR2.sa_handler = manejador_SIGUSR2;
    act_SIGUSR1.sa_handler = manejador_SIGUSR1;
    sigemptyset(&(act_SIGINT.sa_mask));
    sigemptyset(&(act_SIGUSR1.sa_mask));
    sigemptyset(&(act_SIGUSR2.sa_mask));
    act_SIGINT.sa_flags = 0;
    act_SIGUSR1.sa_flags = 0;
    act_SIGUSR2.sa_flags = 0;

    // Ignoramos la señal SIGUSR2 (ya que el proceso se estará cerrando)
    sigaddset(&(act_SIGINT.sa_mask), SIGUSR2);

    /* Establecemos los manejadores */
    if (sigaction(SIGINT, &act_SIGINT, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR1, &act_SIGUSR1, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR2, &act_SIGUSR2, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Creamos/accedemos a la red */
    NetData *net = create_net();
    if (net == NULL) {
        fprintf(stderr, "Error al crear/acceder a la red de mineros.");
        exit(EXIT_FAILURE);
    }
    
    index = net_get_index(net);
    if (index == -1) {
        fprintf(stderr, "Error al obtener el indice donde nos encontramos en la red.\n");
        close_net(net);
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
            
        }

        /* Votación */
        if (sig_usr2_recibida == 1) {
            while(sem_wait(&net->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }
            while(sem_wait(&sbi->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }

            /* Cogemos la solución */
            long int solution = sbi->solution;

            /* Comprobamos la solución y escribimos el resultado */
            if (simple_hash(solution) == sbi->target) net->voting_pool[index] = 1;
            else net->voting_pool[index] = 0;
            net->num_voters += 1;
            printf("Comparamos la solución %ld==%ld\n", simple_hash(solution), block->target);

            printf("Numero votantes %d\n", net->num_voters);

            /* Si han votado todos, dejamos al ganador hacer el recuento */
            if (net->num_voters == net->total_miners-1)
                sem_post(&net->check_voting);

            sem_post(&sbi->mutex);
            sem_post(&net->mutex);

            sig_usr2_recibida = 0;

            /* Los votantes se bloquean que el recuento se haga */
            while(sem_wait(&sbi->voters_update_block) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }
            while(sem_wait(&sbi->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }

            printf("Actualizamos la solución del bloque.\n");
            if (sbi->is_valid == 1) block->solution = solution;

            sem_post(&sbi->mutex);

            /* Bloqueamos a los votantes hasta que el ganador inicie la nueva ronda */
            while(sem_wait(&sbi->next_round) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }
        } else if (threads_info[i].solution != -1) {
            while(sem_wait(&net->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }
            while(sem_wait(&sbi->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }
            
            block->solution  = threads_info[i].solution;

            /* Obtenemos el quorum */
            int quorum  = get_quorum(net);
            printf("Obtenemos el quorum: %d\n", quorum); // BORRAR
            if (quorum == -1) {
                sem_post(&net->mutex);
                sem_post(&sbi->mutex);
                fprintf(stderr, "Error al obtener el quorum en get_quorum");
                close_net(net);
                close_shared_block_info(sbi);
                block_destroy_blockchain(block);
                free(threads_info);
            }

            /* Enviamos a todos los mineros sigusr2 */
            for (int k = 0; k < MAX_MINERS; k++) {
                if (net->miners_pid[k] != pid && net->miners_pid[k] != -1) {// BORRAR
                    kill(net->miners_pid[k], SIGUSR2);
                    printf("Mandamos SIGUSR2 a %d\n", (int)net->miners_pid);// BORRAR
                }// BORRAR
            }

            /* Actualizamos el número total de mineros, +1 por el proceso ganador */
            net->total_miners = quorum+1;
            printf("total_miners: %d\n", net->total_miners);

            sem_post(&sbi->mutex);
            sem_post(&net->mutex);

            /* Proceso ganador bloqueado esperando a confirmación */
            if (quorum >= 1) sem_wait(&net->check_voting);
            printf("Comprobamos los votos\n");

            while(sem_wait(&net->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }
            while(sem_wait(&sbi->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }

            /* Contamos los votos */
            int yes_votes = 0;
            for (int k = 0; k < MAX_MINERS; k++)
                if (net->voting_pool[k] == 1) 
                    yes_votes += 1;

            printf("Contamos los votos %d / %d quorum\n", yes_votes, quorum); // BORRAR

            /* Si hay mayoría actualizamos el target */
            if (quorum == 0) {
                net->last_winner = getpid();
                sbi->is_valid = 1;
                sbi->target = block->solution;
                sbi->solution = block->solution;
            } else if (yes_votes/quorum > 0.5) {
                net->last_winner = getpid();
                sbi->is_valid = 1;
                sbi->target = block->solution;
                sbi->solution = block->solution;
            } else sbi->is_valid = 0;
            
            sem_post(&sbi->mutex);
            sem_post(&net->mutex);

            /* Desbloqueando a los votantes para que actualicen sus bloques */
            if (quorum >= 1) {
                for (int k = 0; k < quorum; k++)
                    sem_post(&sbi->voters_update_block);
                
                /* Bloqueamos al proceso ganador hasta que los demas esten listos para empezar */
                sem_wait(&net->start_next_round); 
            }

            /* Reseteamos la voting pool y el bloque usadas */
            while(sem_wait(&net->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }
            while(sem_wait(&sbi->mutex) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    free(threads_info);
                    close_net(net);
                    close_shared_block_info(sbi);
                    exit(EXIT_FAILURE);
                }
            }

            /* Reseteando la voting pool e is_valid */
            for (int k = 0; k < MAX_MINERS; k++)
                net->voting_pool[k] = -1;
            
            sbi->is_valid = 0;

            printf("Reseteamos valores\n");// BORRAR

            sem_post(&sbi->mutex);
            sem_post(&net->mutex);

            /* Desbloqueando a los votantes para que actualicen su bloque */
            if (quorum >= 1) {
                for (int k = 0; k < quorum; k++)
                sem_post(&sbi->next_round);
            }
        }

        /* Abandonamos el bucle principal si se ha recibido SIGINT */
        if (sig_int_recibida == 1) break;

        /* No debería ejecutarse nunca */
        if (block->solution == -1) {
            printf("No se ha encontrado la solución.\n");
            break;
        }

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
