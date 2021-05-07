/**
 * @file sems.c
 * @author kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se codifica el comportamiento
 * de los semáforos.
 * @version 0.1 - Votación y concurrencia.
 * @date 2021-05-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "sems.h"

Sems *sems_ini() {
    Sems *sems = NULL;
    int fd_shm;

    /* Creamos la memoria compartida de la red */
    if ((fd_shm = shm_open(SHM_SEMS, O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR)) == -1) {
        /* En el caso de que la memoria compartida exista llama a link */
        if(errno == EEXIST) return link_shared_sems();
        perror("shm_open");
        return NULL;
    }

    if (ftruncate(fd_shm, sizeof(Sems)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_SEMS);
        return NULL;
    }

    sems = mmap(NULL, sizeof(Sems), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (sems == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_SEMS);
        return NULL;
    }

    /* Inicializando semáforos */
    if (sem_init(&sems->net_mutex, 1, 1) == -1 
    || sem_init(&sems->block_mutex, 1, 1) == -1
    || sem_init(&sems->mutex, 1, 1) == -1
    || sem_init(&sems->vote, 1, 0) == -1
    || sem_init(&sems->count_votes, 1, 0) == -1
    || sem_init(&sems->update_blocks, 1, 0) == -1
    || sem_init(&sems->update_target, 1, 0) == -1
    || sem_init(&sems->finish, 1, 0) == -1
    ) {
        perror("sem_init");

        munmap(sems, sizeof(Sems));
        shm_unlink(SHM_SEMS);

        return NULL;
    }

    /* Inicializando numéro de mineros */
    sem_down(&sems->mutex);
    sems->total_miners = 1;
    sems->blocked_loosers = 0;
    sem_up(&sems->mutex);

    return sems;
}

Sems *link_shared_sems() {
    Sems *sems = NULL;
    int fd_shm;

    /* Open of the shared memory. */
    if ((fd_shm = shm_open(SHM_SEMS, O_RDWR, 0)) == -1) {
        perror("shm_open");
        return NULL;
    }

    /* Mapping of the memory segment. */
    sems = mmap(NULL, sizeof(Sems), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (sems == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    /* Inicializando variables */
    sem_down(&sems->mutex);
    sems->total_miners += 1;
    sem_up(&sems->mutex);

    return sems;
}

int sem_down(sem_t *s) {
    if (s == NULL) return -1;

    /* Inicializamos variables */
    while(sem_wait(s) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            return -1;
        }
    }
    return 0;
}

int sem_up(sem_t *s) {
    if (s == NULL) return -1;
    sem_post(s);
    return 0;
}

void close_sems(Sems *sems) {
    short bool_borrar = 0;

    if (sems == NULL) return;

    sem_down(&sems->mutex);
    sems->total_miners -= 1;
    if (sems->total_miners == 0) bool_borrar = 1;
    sem_up(&sems->mutex);

    munmap(sems, sizeof(Sems));

    /* En caso de que seamos los últimos en abandonar la red la destruimos */
    if (bool_borrar == 1) {
        sem_destroy(&sems->net_mutex);
        sem_destroy(&sems->block_mutex);
        sem_destroy(&sems->mutex);
        sem_destroy(&sems->count_votes);
        sem_destroy(&sems->vote);
        sem_destroy(&sems->update_blocks);
        sem_destroy(&sems->update_target);
        sem_destroy(&sems->finish);
        shm_unlink(SHM_SEMS);
    }
}