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

    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    act_SIGINT.sa_handler = manejador_SIGINT;
    act_SIGALRM.sa_handler = manejador_SIGALRM;

    /* Establecemos los manejadores */
    if(sigaction(SIGINT, &act_SIGINT, NULL) < 0
    || sigaction(SIGALRM, &act_SIGALRM, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Creando un hijo */
    pid_hijo = fork();
    if (pid_hijo == -1) {
        perror("fork");
        return -1;
    } else if (pid_hijo == 0) { /* Ejecución del hijo */
        Block received_block;
        Block *chain = NULL;

        close(fd[1]); /* Cerramos el extremo de escritura */
        time_t next_alrm = time(NULL) + 5;

        while (1) {
            if (time(NULL) > next_alrm) {
                alarm(5);
                next_alrm = 0;
            }

            if (sig_int_recibida == 1) break;

            /* Leemos el bloque */
            int nbytes = read(fd[0], &received_block, sizeof(received_block));
            if (nbytes == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            /* Hacemos una copia y la guardamos en nuestra cadena dinámica */
            Block *aux = block_ini();
            if (aux == NULL) {
                fprintf(stderr, "Error al hacer block_ini\n");
                exit(EXIT_FAILURE);
            }
            if (block_copy(&received_block, aux) == -1) {
                fprintf(stderr, "Error en block_copy\n");
                exit(EXIT_FAILURE);
            }
            if (block_set(chain, aux) == -1) {
                fprintf(stderr, "Error en block_set\n");
                exit(EXIT_FAILURE);
            }
            chain = aux;

            if (sig_alrm_recibida == 1) {
                sig_alrm_recibida = 0;
                /* Escribimos en el archivo */
            }

        }
    } else { /* Ejecución del padre */

        close(fd[0]); /* Cerramos extremo de lectura */

        Block *buffer_blocks[BUFFER_SIZE]; /* Buffer de bloques */
        short index = 0;

        /* Inicializamos punteros */
        for (int i = 0; i < BUFFER_SIZE; i++) buffer_blocks[i] = NULL;

        /* Abrimos la cola de mensajes */
        mqd_t queue = mq_open(MQ_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
        if (queue == (mqd_t)-1) {
            perror("mq_open");
            exit(EXIT_FAILURE);
        }

        while (1) {
            Mensaje msg;

            if (sig_int_recibida) break;

            /* Recibimos la instruccion mandada */
            if (mq_receive(queue, (char *)&msg, sizeof(Mensaje), NULL) == -1){
                perror("mq_receive");
                mq_close(queue);
                exit(EXIT_FAILURE);
            }

            /* Comprobamos si el bloque ya esta en el buffer */
            short is_in = 0;
            for (int i = 0; i < BUFFER_SIZE; i++) 
                if (buffer_blocks[i] != NULL) 
                    if (buffer_blocks[i]->id == msg.block.id)
                        is_in = 1;

            if (is_in == 1) {
                /* Imprimimos el mensaje que toque */
                if (simple_hash(msg.block.solution) == msg.block.target)
                    printf("Verified block %d with solution %ld for target %ld\n", msg.block.id);
                else printf("Error in block %d with solution %ld for target %ld\n", msg.block.id);

                Block b_copy;
                if (block_copy(&msg.block, &b_copy) == -1) {
                    mq_close(queue);
                    exit(EXIT_FAILURE);
                }

                /* Escribimos la copia del bloque en la tubería */
                int nbytes = write(fd[1], &b_copy, strlen(b_copy));
                if (nbytes == -1) {
                    perror("write");
                    mq_close(queue);
                    exit(EXIT_FAILURE);
                }
            } else {
                /* Metemos el bloque en el buffer */
                buffer_blocks[index] = &msg.block;
                index = (index+1)%BUFFER_SIZE;
            }
        }

        kill(pid_hijo, SIGINT);

        /* Cerrando la cola de mensajes */
        mq_close(queue);
    }

    return 0;
}