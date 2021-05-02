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
 *          0.5 - Votación y concurrencia.
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
    printf("%d\n", pid);

    /* Inicializamos una máscara para ignorar SIGINT durante la inicialización.
    Inicializamos también una máscara para esperar SIGUSR2. */
    sigset_t mask, wait_for_winner, ignore_all;
    sigemptyset(&mask);
    sigfillset(&wait_for_winner);
    sigfillset(&ignore_all);
    sigaddset(&mask, SIGINT);
    sigdelset(&wait_for_winner, SIGINT);
    sigdelset(&wait_for_winner, SIGUSR2);

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

    /* Creamos/cargamos semáforos */
    Sems *sems = sems_ini();
    if (sems == NULL) {
        fprintf(stderr, "Error al crear/cargar los semáforos.\n");
        exit(EXIT_FAILURE);
    }

    /* Creamos/accedemos a la red */
    sem_down(&sems->net_mutex);
    NetData *net = create_net();
    if (net == NULL) {
        fprintf(stderr, "Error al crear/acceder a la red de mineros.\n");
        close_sems(sems);
        exit(EXIT_FAILURE);
    }
    
    index = net_get_index(net);
    if (index == -1) {
        fprintf(stderr, "Error al obtener el indice donde nos encontramos en la red.\n");
        
        sem_down(&sems->net_mutex);
        close_net(net);
        sem_up(&sems->net_mutex);
        
        close_sems(sems);
        exit(EXIT_FAILURE);
    }
    sem_up(&sems->net_mutex);

    /* Generamos un target aleatorio entre 1 - 1.000.000 */
    srand(time(NULL));

    /* Establecemos un target y el número de trabajadores */
    num_workers = atoi(argv[1]);
    rounds = atol(argv[2]);

    /* En caso de que el número de rondas sea infinito */
    if (rounds <= 0) infinite = 1;

    if (num_workers > MAX_WORKERS) {
        fprintf(stderr, "Número incorrecto de trabajadores. Defina un número entre [1-10] (ambos incluidos).\n");
        
        sem_down(&sems->net_mutex);
        close_net(net);
        sem_up(&sems->net_mutex);

        close_sems(sems);

        exit(EXIT_FAILURE);
    }

    /* Creamos/linkeamos la memoria compartida */
    shared_block_info *sbi = create_shared_block_info();
    if (sbi == NULL) {
        fprintf(stderr, "Error al crear/linkear la memoria compartida.\n");
        
        sem_down(&sems->net_mutex);
        close_net(net);
        sem_up(&sems->net_mutex);

        close_sems(sems);
        
        exit(EXIT_FAILURE);
    }


    /* Reservamos memoria para la estructura usada por los threads  */
    threads_info = (worker_struct*)malloc(num_workers*(sizeof(worker_struct)));
    if (threads_info == NULL) {
        perror("Error reservando memoria para la estructura de los trabajadores. malloc");
        
        sem_down(&sems->net_mutex);
        close_net(net);
        sem_up(&sems->net_mutex);

        sem_down(&sems->block_mutex);
        close_shared_block_info(sbi);
        sem_up(&sems->block_mutex);

        close_sems(sems);

        exit(EXIT_FAILURE);
    }

    /* Como ya está todo inicializado volvemos a permitir la señal SIGINT */
    if(sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        free(threads_info);

        sem_down(&sems->net_mutex);
        close_net(net);
        sem_up(&sems->net_mutex);

        sem_down(&sems->block_mutex);
        close_shared_block_info(sbi);
        sem_up(&sems->block_mutex);

        close_sems(sems);

        exit(EXIT_FAILURE);
    }

    if (sigprocmask(SIG_BLOCK, &ignore_all, NULL) == -1) {
        perror("sigprocmask");
        free(threads_info);

        sem_down(&sems->net_mutex);
        close_net(net);
        sem_up(&sems->net_mutex);

        sem_down(&sems->block_mutex);
        close_shared_block_info(sbi);
        sem_up(&sems->block_mutex);

        close_sems(sems);

        exit(EXIT_FAILURE);
    }

    /* Ejecutando las rondas correspondientes */
    for (int n = 0; n < rounds || infinite == 1; n++) {

        /* Creamos el bloque */
        block = block_ini();
        if (block == NULL) {
            fprintf(stderr, "Error creando el bloque. block_ini.\n");
            free(threads_info);
            
            sem_down(&sems->net_mutex);
            close_net(net);
            sem_up(&sems->net_mutex);

            sem_down(&sems->block_mutex);
            close_shared_block_info(sbi);
            sem_up(&sems->block_mutex);

            block_destroy_blockchain(last_block);

            close_sems(sems);

            exit(EXIT_FAILURE);
        }

        if (block_set(last_block, block) == -1) {
            fprintf(stderr, "Error inicializando el bloque. block_set.\n");
            free(threads_info);
            
            sem_down(&sems->net_mutex);
            close_net(net);
            sem_up(&sems->net_mutex);

            sem_down(&sems->block_mutex);
            close_shared_block_info(sbi);
            sem_up(&sems->block_mutex);

            block_destroy_blockchain(block);

            close_sems(sems);

            exit(EXIT_FAILURE);
        }

        /* Controlamos el valor de la memoria concurrente el semáforo mutex */
        // SEMáforo
        if (sbi->target != block->target) block->target = sbi->target;

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
                
                sem_down(&sems->net_mutex);
                close_net(net);
                sem_up(&sems->net_mutex);

                sem_down(&sems->block_mutex);
                close_shared_block_info(sbi);
                sem_up(&sems->block_mutex);

                block_destroy_blockchain(block);

                close_sems(sems);

                exit(EXIT_FAILURE);
            }
        }

        /* Terminando la ejecución de los threads */
        short index_ganador = -1;
        for (i = 0; i < num_workers; i++) {

            err = pthread_join(threads[i], NULL);
            if (err != 0) {
                perror("pthread_join");
                free(threads_info);
                
                sem_down(&sems->net_mutex);
                close_net(net);
                sem_up(&sems->net_mutex);

                sem_down(&sems->block_mutex);
                close_shared_block_info(sbi);
                sem_up(&sems->block_mutex);

                block_destroy_blockchain(block);

                close_sems(sems);

                exit(EXIT_FAILURE);
            }

            if (threads_info[i].solution != -1) index_ganador = i;
        }

        /* Comprobamos si alguien ha propuesto una solución */
        short is_founded = 0;
        sem_down(&sems->block_mutex);
        if (sbi->solution != -1) is_founded = 1; // Solución encontrada pero no se ha enviado
        else if (sbi->solution == 0);
        else sbi->solution = 0;
        sem_up(&sems->block_mutex);

        /* Se ha encontrado una solución, suspendemos al proceso hasta que reciba */
        if (is_founded == 1 && sig_usr2_recibida == 0 && sig_int_recibida == 0) sigsuspend(&wait_for_winner);

        /* Abandonamos el bucle principal si se ha recibido SIGINT */
        if (sig_int_recibida == 1) break;
        else if (sig_usr2_recibida == 1) { /* Votación */
            int quorum = get_quorum(net);
            sig_usr2_recibida = 0;
            int total_voters = 0;
            printf("[%d] Soy perdedor.\n", index);

            /* 7. El minero se bloquea antes votar */
            if (quorum > 0) sem_down(&sems->vote);

            /* 8. El minero comprueba el resultado */
            short result = -1; 
            sem_down(&sems->block_mutex);
            if (sbi->target == simple_hash(sbi->solution)) result = 1; // Voto positivo
            else result = 0; // Voto negativo
            sem_up(&sems->block_mutex);

            /* 9. El minero introduce su voto */
            sem_down(&sems->net_mutex);
            net->voting_pool[index] = result;

            /* 10. Si somos el último en votar dejamos que cuente los votos el ganador */
            total_voters = sems->quorum;
            if (count_votes(net) == total_voters && quorum > 0) sem_up(&sems->count_votes);
            sem_up(&sems->net_mutex);

            /* 11. Los mineros se bloquean esperando a que se cuenten los votos */
            if (quorum > 0) sem_down(&sems->update_blocks);

            /* 16. Actualizamos nuestro bloque */
            short err = 0;
            sem_down(&sems->block_mutex);
            if (sbi->is_valid == 1) err = update_block(sbi, block);
            else {
                /* 15.1 Destruimos el bloque */
                Block *aux = NULL;

                aux = block;
                block = aux->prev;
                block_destroy(aux);
            }

            /* 17. Dejamos al proceso ganador actualizar el nuevo target */
            sem_down(&sems->net_mutex);
            sem_down(&sems->mutex);
            sems->finished_miners += 1;
            if (sems->finished_miners == total_voters) {
                if (quorum > 0) sem_post(&sems->update_target);
                sems->finished_miners = 0;
            }
            sem_up(&sems->mutex);
            sem_up(&sems->net_mutex);
            sem_up(&sems->block_mutex);

            /* 18. Esperamos a que el nuevo target haya sido actualizado */
            if (quorum > 0) sem_down(&sems->finish);
            
        } else if (index_ganador != -1) { /* Ganador */
            printf("[%d] Soy ganador.\n", index);

            /* 1. El ganador actualiza la solución */
            sem_down(&sems->block_mutex);
            sbi->solution = threads_info[index_ganador].solution;
            sem_up(&sems->block_mutex);

            /* 2. El ganador obtiene el quorum */
            short quorum = 0;
            
            sem_down(&sems->net_mutex);
            sem_down(&sems->mutex);
            sems->quorum = get_quorum(net);
            quorum = sems->quorum;
            sem_up(&sems->mutex);
            sem_up(&sems->net_mutex);

            if (quorum == -1) {
                fprintf(stderr, "Error en get_quorum.\n");
                free(threads_info);
                
                sem_down(&sems->net_mutex);
                close_net(net);
                sem_up(&sems->net_mutex);

                sem_down(&sems->block_mutex);
                close_shared_block_info(sbi);
                sem_up(&sems->block_mutex);

                block_destroy_blockchain(block);

                close_sems(sems);

                exit(EXIT_FAILURE);
            }

            /* 3. El ganador actualiza el número de mineros activos. +1 incluyendo al ganador */
            sem_down(&sems->net_mutex);
            net->total_miners = quorum + 1;

            /* 4. El ganador envía SIGUSR2 */
            send_SIGUSR2(net);
            sem_up(&sems->net_mutex);

            /* 5. Dejamos que los votantes empiezen a votar */
            for(int k = 0; k < quorum; k++) sem_up(&sems->vote);

            /* 6. El ganador espera a que se vote bloqueandose */
            if (quorum > 0) sem_down(&sems->count_votes);

            /* 12. Contamos los votos */
            int positive_votes = 0;
            sem_down(&sems->net_mutex);
            for (int k = 0; k < MAX_MINERS; k++) if (net->voting_pool[k] == 1) positive_votes++;

            /* 13. Establecemos si es valido el bloque */
            short err = 0;
            sem_down(&sems->block_mutex);
            if (quorum > 0) {    
                if (positive_votes/quorum >= 0.5) {
                    printf("[%d] GANADOR Votación exitosa.\n", index);

                    /* 13.1 En caso de exito ponemos is_valid a 1 */
                    sbi->is_valid = 1;

                    /* 13.2 Actualizamos los campos respectivos en la red */
                    net->last_winner = index;

                    /* 13.3 Actualizamos las wallets */
                    sbi->wallets[index] += 1;

                    /* 13.4 Actualizamos nuestro bloque de forma local */
                    err = update_block(sbi, block);

                } else {
                    printf("[%d] GANADOR Votación fallida.\n", index);

                    Block *aux = NULL;

                    // 13.5 Si no es valido, destruimos el bloque actual
                    sbi->is_valid = 0; 

                    aux = block;
                    block = aux->prev;
                    block_destroy(aux);
                }

                /* Reseteamos la votación */
                for (int k = 0; k < MAX_MINERS; k++) net->voting_pool[k] = -1;
            } else {
                sbi->is_valid = 1;
                net->last_winner = index;
                sbi->wallets[index] += 1;
                err = update_block(sbi, block);
            }

            sem_up(&sems->block_mutex);
            sem_up(&sems->net_mutex);

            /* 14. Desbloqueamos a los mineros para que actualicen su bloque */
            for (int k = 0; k < quorum; k++) sem_up(&sems->update_blocks);

            if (err == -1) {
                /* Desbloqueamos a los mineros */
                for (int k = 0; k < quorum; k++) {
                    sem_up(&sems->update_blocks);
                    sem_up(&sems->finish);
                }

                fprintf(stderr, "Error en update_block.\n");
                free(threads_info);
                
                sem_down(&sems->net_mutex);
                close_net(net);
                sem_up(&sems->net_mutex);

                sem_down(&sems->block_mutex);
                close_shared_block_info(sbi);
                sem_up(&sems->block_mutex);

                block_destroy_blockchain(block);

                close_sems(sems);

                exit(EXIT_FAILURE);                
            }

            /* 15. Esperamos a que los mineros hayan actualizado su bloque */
            if (quorum > 0) sem_down(&sems->update_target);

            /* 19. Actualizamos el bloque compartido */
            sem_down(&sems->block_mutex);
            printf("[%d] GANADOR Nuevo target: %ld\n", index, sbi->solution);

            sbi->is_valid = 0;
            sbi->target = sbi->solution;
            sbi->solution = -1;
            sem_up(&sems->block_mutex);

            /* 20. Acabamos el proceso de votación liberando a los votantes */
            for (int k = 0; k < quorum; k++) sem_up(&sems->finish);
        }

        /* No debería ejecutarse nunca */
        if (block->solution == -1) {
            printf("No se ha encontrado la solución.\n");
            break;
        }

        last_block = block;
        solution_find = 0;
    }

    /* Liberamos recursos */
    sem_down(&sems->net_mutex);
    close_net(net);
    sem_up(&sems->net_mutex);

    sem_down(&sems->block_mutex);
    close_shared_block_info(sbi);
    sem_up(&sems->block_mutex);

    close_sems(sems);

    block_destroy_blockchain(block);
    free(threads_info);

    exit(EXIT_FAILURE);
}
