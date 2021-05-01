/**
 * @file net.c
 * @author kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se codifica el comportamiento
 * de la red de mineros
 * @version 0.1 - Implementación de la red.
 * @date 2021-05-01
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "net.h"

NetData *create_net() {
    NetData *nd = NULL;
    int fd_shm;

    /* Creamos la memoria compartida de la red */
    if ((fd_shm = shm_open(SHM_NAME_NET, O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR)) == -1) {
        /* En el caso de que la memoria compartida exista llama a link */
        if(errno == EEXIST) return link_shared_net();
        perror("shm_open");
        return NULL;
    }

    if (ftruncate(fd_shm, sizeof(NetData)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME_NET);
        return NULL;
    }

    nd = mmap(NULL, sizeof(NetData), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (nd == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME_NET);
        return NULL;
    }

    /* Inicializamos el semáforo */
    if (sem_init(&nd->mutex, 1, 1) == -1) {
        perror("sem_init");

        munmap(nd, sizeof(NetData));
        shm_unlink(SHM_NAME_NET);

        return NULL;
    }

    if (sem_init(&nd->check_voting, 1, 0) == -1) {
        perror("sem_init");

        munmap(nd, sizeof(NetData));
        shm_unlink(SHM_NAME_NET);

        return NULL;
    }

    if (sem_init(&nd->start_next_round, 1, 0) == -1) {
        perror("sem_init");

        munmap(nd, sizeof(NetData));
        shm_unlink(SHM_NAME_NET);

        return NULL;
    }

    pid_t pid = getpid(); 

    /* Inicializamos variables */
    while(sem_wait(&nd->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            close_net(nd);
            return NULL;
        }
    }
    nd->last_miner = pid;
    nd->last_winner = -1;
    nd->num_voters = 0;
    
    /* Inicializando PIDs a -1 */
    for (int i = 0; i < MAX_MINERS; i++)
        nd->miners_pid[i] = -1;

    nd->miners_pid[0] = getpid();
    nd->total_miners = 1;
    for (int i = 0; i < MAX_MINERS; i++)
        nd->voting_pool[i] = -1;
    sem_post(&nd->mutex);

    return nd;
}

NetData *link_shared_net() {
    NetData *nd = NULL;
    int fd_shm;

    /* Open of the shared memory. */
    if ((fd_shm = shm_open(SHM_NAME_NET, O_RDWR, 0)) == -1) {
        perror("shm_open");
        return NULL;
    }

    /* Mapping of the memory segment. */
    nd = mmap(NULL, sizeof(NetData), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (nd == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    pid_t pid = getpid(); 

    /* Inicializamos variables */
    while(sem_wait(&nd->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            close_net(nd);
            return NULL;
        }
    }
    nd->last_miner = pid;
    nd->miners_pid[nd->total_miners] = getpid();
    nd->total_miners += 1;
    sem_post(&nd->mutex);

    return nd;
}

int net_get_index(NetData *net) {
    pid_t pid = getpid();
    int index = -1;
    
    while(sem_wait(&net->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            return -1;
        }
    }  
    for (int i = 0; i < net->total_miners; i++) {
        if (pid == net->miners_pid[i]) {
            index = pid;
            break;
        }
    }
    sem_post(&net->mutex);
    
    return index;
}

int get_quorum(NetData *nd) {
    pid_t pid = -1;
    int quorum = 0;

    if (nd == NULL) return -1;

    pid = getpid();

    /* Enviando SIGUSR1 a todos los procesos */
    for (int i = 0; i < MAX_MINERS; i++) {
        if (nd->miners_pid[i] != -1 && nd->miners_pid[i] != pid) 
            if (kill(nd->miners_pid[i], SIGUSR1) != -1) 
                quorum += 1;
    }

    return quorum;
}

int close_net(NetData *nd) {
    short bool_borrar = 0;

    if (nd == NULL) return -1;

    /* Inicializamos variables */
    while(sem_wait(&nd->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            close_net(nd);
            return -1;
        }
    }
    nd->total_miners -= 1;
    if (nd->total_miners == 0) bool_borrar = 1;
    sem_post(&nd->mutex);

    /* En caso de que seamos los últimos en abandonar la red la destruimos */
    if (bool_borrar == 1) {
        sem_destroy(&nd->mutex);
        sem_destroy(&nd->check_voting);
        sem_destroy(&nd->start_next_round);
        munmap(nd, sizeof(NetData));
        shm_unlink(SHM_NAME_NET);
    }
    return 0;
}