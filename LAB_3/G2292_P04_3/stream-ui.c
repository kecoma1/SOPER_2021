/**
 * @file stream-ui.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se define el comportamiento de la UI del streaming.
 * Aquí se crean los procesos server y client, y se manejan las instrucciones
 * del usuario.
 * @version 1.0
 * @date 2021-04-13
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "ui_struct.h"

/**
 * @brief Función para liberar todos los recursos.
 * Esta función debe ser usada una vez se han inicializado
 * todas las estructuras necesarias para la ejecución.
 * 
 * @param queue_server Cola de mensajes que se comunica con el servidor.
 * @param queue_client Cola de mensajes que se comunica con el cliente.
 * @param ui_shared Estructura compartida entre procesos
 */
void release_resources(mqd_t queue_server, mqd_t queue_client, ui_struct *ui_shared) {
    /* Liberando recursos */
    munmap(ui_shared, sizeof(ui_struct));
    shm_unlink(SHM_NAME);

    /* Destruyendo las colas de mensajes */
    mq_close(queue_client);
    mq_close(queue_server);
    mq_unlink(MQ_NAME_SERVER);
    mq_unlink(MQ_NAME_CLIENT);

    /* Destruyendo semaforos */
    sem_destroy(&ui_shared->sem_fill);
    sem_destroy(&ui_shared->sem_empty);
    sem_destroy(&ui_shared->sem_mutex);
}

int main(int argc, char *argv[]) {
    pid_t pid_ui = 0, pid_server = 0, pid_client = 0;
    char input[INPUTMAXSIZE] = "\0";
    int fd_shm = 0;
    ui_struct *ui_shared = NULL;
    Mensaje msg;

    if (argc < 3) {
        printf("Se deben incluir un fichero de entrada y otro de salida.\n./stream-ui <salida> <entrada>\n");
        return -1;
    }

    pid_ui = getppid();

    /* Abrimos la memoria compartida */
    if ((fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR)) == -1) {
        perror("shm_open");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Reajustamos el tamaño de la memoria compartida */
    if (ftruncate(fd_shm, sizeof(ui_struct)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Mapeamos la memoria compartida en una estructura */
    ui_shared = mmap(NULL, sizeof(ui_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (ui_shared == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    
    /* Inicializando variables */
    strcpy(ui_shared->buffer, "00000");
    ui_shared->post_pos = 0;
    ui_shared->get_pos = 0;

    /* Inicializando semáforos */
    if (sem_init(&ui_shared->sem_fill, 1, 0) == -1) {
        perror("sem_init");

        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);

        exit(EXIT_FAILURE);
    }
    if (sem_init(&ui_shared->sem_empty, 1, 1) == -1) {
        perror("sem_init");

        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);

        sem_destroy(&ui_shared->sem_fill);

        exit(EXIT_FAILURE);
    }
    if (sem_init(&ui_shared->sem_mutex, 1, 1) == -1) {
        perror("sem_init");

        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);

        sem_destroy(&ui_shared->sem_fill);
        sem_destroy(&ui_shared->sem_empty);

        exit(EXIT_FAILURE);
    }

    /* Inicializamos la estructura de la cola de mensajes */
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(Mensaje)
    };

    /* Abrimos las colas de mensajes */
    mqd_t queue_server = mq_open(MQ_NAME_SERVER, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
    if (queue_server == (mqd_t)-1) {
        perror("mq_open");

        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);

        sem_destroy(&ui_shared->sem_empty);
        sem_destroy(&ui_shared->sem_fill);
        sem_destroy(&ui_shared->sem_mutex);

        exit(EXIT_FAILURE);
    }

    mqd_t queue_client = mq_open(MQ_NAME_CLIENT, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
    if (queue_client == (mqd_t)-1) {
        perror("mq_open");

        mq_close(queue_server);
        mq_unlink(MQ_NAME_SERVER);

        munmap(ui_shared, sizeof(ui_struct));
        shm_unlink(SHM_NAME);

        sem_destroy(&ui_shared->sem_empty);
        sem_destroy(&ui_shared->sem_fill);
        sem_destroy(&ui_shared->sem_mutex);

        exit(EXIT_FAILURE);
    }

    /* Creamos el proceso server */
    pid_server = fork();
    if (pid_server == -1) {
        perror("fork");
        release_resources(queue_server, queue_client, ui_shared);
        exit(EXIT_FAILURE);
    }
    /* Ejecución del proceso server */
    if (pid_server == 0) {
        if (execl("./stream-server", argv[2], (char*) NULL) == -1) {
            perror("execl");
            release_resources(queue_server, queue_client, ui_shared);
            exit(EXIT_FAILURE);
        }
    } 

    /* Solo el padre puede encargarse de crear al cliente */
    if (pid_ui == getppid()) {
        /* Creamos el proceso client */
        pid_client = fork();
        if (pid_client == -1) {
            perror("fork");
            release_resources(queue_server, queue_client, ui_shared);
            exit(EXIT_FAILURE);
        }
    }
    /* Ejecución del proceso client */
    if (pid_client == 0) {
        if (execl("./stream-client", argv[1], (char*) NULL) == -1) {
            perror("execl");
            release_resources(queue_server, queue_client, ui_shared);
            exit(EXIT_FAILURE);
        }
    }

    /* Recogemos los comandos */
    while(1) {
        /* Recogiendo la input de stdin */
        printf(">>> ");
        fgets(input, INPUTMAXSIZE, stdin);

        /* Marcando el final de la string */
        int len = strlen(input);
        if (len == INPUTMAXSIZE)
            input[INPUTMAXSIZE-1] = '\0';
        else
            input[len] = '\0';
        
        /* Construyendo el mensaje */
        strcpy(msg.message, input);

        /* Comando post */
        if(strncmp(msg.message, "post", 4) == 0){
            /* Mandando mensaje a el stream-server */
            if(mq_send(queue_server, (const char *)&msg, sizeof(msg), 0) == -1){
                perror("execl");
                release_resources(queue_server, queue_client, ui_shared);
                exit(EXIT_FAILURE);
            }
        } /* Comando get */
        else if (strncmp(msg.message, "get", 3) == 0) {
            /* Mandando mensaje al proceso stream-client */
            if(mq_send(queue_client, (const char *)&msg, sizeof(msg), 0) == -1){
                perror("execl");
                release_resources(queue_server, queue_client, ui_shared);
                exit(EXIT_FAILURE);
            }
        } else if (strncmp(msg.message, "exit", 4) == 0) {
            /* Mandando mensajes a ambos procesos */
            if(mq_send(queue_client, (const char *)&msg, sizeof(msg), 0) == -1 
            || mq_send(queue_server, (const char *)&msg, sizeof(msg), 0) == -1) {
                perror("execl");
                release_resources(queue_server, queue_client, ui_shared);
                exit(EXIT_FAILURE);
            }
            break;
        } else {
            printf("Comando invalido. post, get, exit\n");
        }
    }

    /* Esperamos a los procesos */
    waitpid(pid_server, NULL, 0);
    waitpid(pid_client, NULL, 0);
    
    release_resources(queue_server, queue_client, ui_shared);

    return 0;
}