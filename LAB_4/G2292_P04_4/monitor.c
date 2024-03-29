/**
 * @file monitor.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se codifica el comportamiento 
 * del proceso monitor.
 * @version 0.1 - Monitor
 * @date 2021-05-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "monitor.h"

int sig_int_recibida = 0;
int sig_alrm_recibida = 0;

/**
 * @brief Función para manejar la señal SIGINT.
 * 
 * @param sig Señal.
 */
void manejador_SIGINT(int sig) {
    sig_int_recibida = 1;
}

/**
 * @brief Función para manejar la señal SIGALRM.
 * 
 * @param sig Señal.
 */
void manejador_SIGALRM(int sig) {
    sig_alrm_recibida = 1;
}

int main() {
    pid_t pid_padre = 0;
    pid_t pid_hijo = 0;

    int fd[2];

    struct sigaction act_SIGINT, act_SIGALRM;

    /* Inicializamos la estructura de la cola de mensajes */
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(Mensaje)
    };

    pid_padre = getpid();

    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* Establecemos los manejadores */
    act_SIGINT.sa_handler = manejador_SIGINT;
    act_SIGALRM.sa_handler = manejador_SIGALRM;
    sigemptyset(&(act_SIGINT.sa_mask));
    sigemptyset(&(act_SIGALRM.sa_mask));
    act_SIGINT.sa_flags = 0;
    act_SIGALRM.sa_flags = 0;

    /* Creando un hijo */
    pid_hijo = fork();
    if (pid_hijo == -1) {
        perror("fork");
        return -1;
    } else if (pid_hijo == 0) { /* Ejecución del hijo */

        if(sigaction(SIGINT, &act_SIGINT, NULL) < 0
        || sigaction(SIGALRM, &act_SIGALRM, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        Block *last_block = NULL;

        close(fd[1]); /* Cerramos el extremo de escritura */
        time_t next_alrm = time(NULL) + 5;

        FILE *pf = fopen("blockchain.log", "w");
        if (pf == NULL) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        alarm(5);

        while (1) {
            Block received_block;
            if (time(NULL) > next_alrm) {
                alarm(5);
                next_alrm = time(NULL) + 5;
            }

            if (sig_int_recibida == 1) break;

            /* Leemos el bloque */
            int nbytes = read(fd[0], &received_block, sizeof(Block));
            if (errno != EINTR && nbytes == -1) {
                perror("read");
                fclose(pf);
                exit(EXIT_FAILURE);
            }

            /* Hacemos una copia y la guardamos en nuestra cadena dinámica */
            if (nbytes != -1) {
                Block *aux = block_ini();
                if (aux == NULL) {
                    fprintf(stderr, "Error al hacer block_ini\n");
                    fclose(pf);
                    exit(EXIT_FAILURE);
                }
                if (block_copy(&received_block, aux) == -1) {
                    fprintf(stderr, "Error en block_copy\n");
                    fclose(pf);
                    exit(EXIT_FAILURE);
                }
                
                if (last_block != NULL) {
                    last_block->next = aux;
                    aux->prev = last_block;
                } else {
                    aux->prev = NULL;
                    aux->next = NULL;
                }
                aux->next = NULL;
                last_block = aux;

                if (sig_alrm_recibida == 1) {
                    sig_alrm_recibida = 0;

                    /* Escribimos en el archivo */
                    fprintf(pf, "\n########## Mostrando la blockchain. ##########\n");
                    print_blocks_in_file(pf, last_block);
                }
            }
        }
        fclose(pf);
        block_destroy_blockchain(last_block);
        exit(EXIT_SUCCESS);
    } else { /* Ejecución del padre */

        /* Cargamos los semáforos */
        Sems *sems = sems_ini();
        if (sems == NULL) {
            fprintf(stderr, "Error en sem_ini\n");
            kill(pid_hijo, SIGINT);
            waitpid(pid_hijo, NULL, 0);
            exit(EXIT_FAILURE);
        }

        /* Nos unimos a la red */
        sem_down(&sems->net_mutex);
        NetData *net = link_monitor_net();
        if (net == NULL) {
            sem_up(&sems->net_mutex);
            fprintf(stderr, "Error en link_monitor_net, puede que no se haya creado la red.\n");
            kill(pid_hijo, SIGINT);
            waitpid(pid_hijo, NULL, 0);
            close_sems(sems);
            exit(EXIT_FAILURE);
        }
        sem_up(&sems->net_mutex);

        if(sigaction(SIGINT, &act_SIGINT, NULL) < 0
        || sigaction(SIGALRM, &act_SIGALRM, NULL) < 0) {
            perror("sigaction");
            kill(pid_hijo, SIGINT);
            waitpid(pid_hijo, NULL, 0);
            exit(EXIT_FAILURE);
        }

        close(fd[0]); /* Cerramos extremo de lectura */

        Block *buffer_blocks[BUFFER_SIZE]; /* Buffer de bloques */
        short index = 0;

        /* Inicializamos punteros */
        for (int i = 0; i < BUFFER_SIZE; i++) buffer_blocks[i] = NULL;

        /* Abrimos la cola de mensajes */
        mqd_t queue = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
        if (queue == (mqd_t)-1) {
            perror("mq_open");
            kill(pid_hijo, SIGINT);
            waitpid(pid_hijo, NULL, 0);

            sem_down(&sems->net_mutex);
            close_net(net);
            sem_up(&sems->net_mutex);

            close_sems(sems);
            exit(EXIT_FAILURE);
        }

        while (1) {
            Mensaje msg;

            if (sig_int_recibida == 1) break;

            /* Recibimos la instruccion mandada */
            if (mq_receive(queue, (char *)&msg, sizeof(Mensaje), NULL) == -1){
                if (errno != EINTR) {
                    perror("mq_receive");
                    kill(pid_hijo, SIGINT);
                    waitpid(pid_hijo, NULL, 0);

                    mq_close(queue);

                    sem_down(&sems->net_mutex);
                    close_net(net);
                    sem_up(&sems->net_mutex);

                    close_sems(sems);
                    exit(EXIT_FAILURE);
                }
                msg.block.id = -1; // Para no actualizar la cadena
            }
            if (msg.block.id != -1) {
                /* Comprobamos si el bloque ya esta en el buffer */
                short is_in = 0;
                for (int i = 0; i < BUFFER_SIZE; i++) 
                    if (buffer_blocks[i] != NULL) 
                        if (buffer_blocks[i]->id == msg.block.id) {
                            is_in = 1;
                            break;
                        }

                if (is_in == 1) {
                    /* Imprimimos el mensaje que toque */
                    if (simple_hash(msg.block.solution) == msg.block.target)
                        printf("Verified block %d with solution %ld for target %ld\n", msg.block.id, msg.block.solution, msg.block.target);
                    else printf("Error in block %d with solution %ld for target %ld\n", msg.block.id, msg.block.solution, msg.block.target);

                } else {
                    /* Metemos el bloque en el buffer */
                    Block b_copy;
                    block_copy(&msg.block, &b_copy);
                    buffer_blocks[index] = &b_copy;
                    index = (index+1)%BUFFER_SIZE;
                }

                Block b_copy;
                if (block_copy(&msg.block, &b_copy) == -1) {
                    fprintf(stderr, "block_copy\n");
                    kill(pid_hijo, SIGINT);
                    waitpid(pid_hijo, NULL, 0);

                    mq_close(queue);

                    sem_down(&sems->net_mutex);
                    close_net(net);
                    sem_up(&sems->net_mutex);

                    close_sems(sems);
                    exit(EXIT_FAILURE);
                }

                /* Escribimos la copia del bloque en la tubería */
                if (is_in == 0) { // Si no está lo enviamos
                    int nbytes = write(fd[1], &b_copy, sizeof(Block));
                    if (nbytes == -1) {
                        kill(pid_hijo, SIGINT);
                        waitpid(pid_hijo, NULL, 0);

                        perror("write");
                        mq_close(queue);

                        sem_down(&sems->net_mutex);
                        close_net(net);
                        sem_up(&sems->net_mutex);

                        close_sems(sems);

                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        /* Esperamos a nuesto hijo */
        kill(pid_hijo, SIGINT);
        waitpid(pid_hijo, NULL, 0);

        sem_down(&sems->net_mutex);
        close_net(net);
        sem_up(&sems->net_mutex);

        close_sems(sems);

        /* Cerrando la cola de mensajes */
        mq_close(queue);
    }

    return 0;
}