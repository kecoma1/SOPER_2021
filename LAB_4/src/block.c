/**
 * @file block.c
 * @author  Kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se codificán las funciones
 * usadas para manejar bloques.
 * @version 0.1 - Implementación bloques.
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "block.h"

Block *block_ini() {
    Block *block = NULL;

    block = (Block *)malloc(sizeof(Block));
    if (block == NULL) {
        perror("malloc");
        return NULL;
    }

    return block;
}

int block_set(Block *prev, Block *block) {
    Block *last_block = NULL;

    if (block == NULL)
        return -1;
    
    if (prev != NULL) {
        /* Vamos al último bloque de la blockchain */
        last_block = prev;
        while (last_block->next != NULL) {
            last_block = last_block->next;
        }
    }

    /* Inicializamos los datos necesarios */
    if (last_block != NULL) block->id = last_block->id + 1;
    block->next = NULL;
    block->prev = last_block;

    /* Si somos el primer bloque no hay anterior */
    if (last_block != NULL) block->target = last_block->solution;
    else block->target = -1;

    block->solution = -1;
    //block->is_valid = -1;

    /* Inicializando las wallets */
    if (last_block != NULL) {
        /* Somos el último bloque copiamos las wallets */
        for (int i = 0; i < MAX_MINERS; i++) 
            block->wallets[i] = last_block->wallets[i];
    } else {
        /* Somos el primer bloque, nuevas wallets */
        for (int i = 0; i < MAX_MINERS; i++) 
            block->wallets[i] = 0;
    }
    
    if (last_block != NULL) last_block->next = block;

    return 0;
}

void block_destroy(Block *block) {
    if (block == NULL)
        return;

    free(block);
    block = NULL;    
}

void block_destroy_blockchain(Block *block) {
    Block *aux = NULL, *last_block = NULL;
    if (block == NULL)
        return;
    
    /* Vamos al último bloque de la blockchain */
    last_block = block;
    while (last_block->next != NULL)
        last_block = last_block->next;

    /* Borramos la blockchain desde el final */
    do {
        aux = last_block->prev;
        block_destroy(last_block);
        last_block = aux;
    } while (aux != NULL);
}

shared_block_info *create_shared_block_info() {
    shared_block_info *sbi = NULL;
    int fd_shm;
    
    /* Creation of the shared memory. */
    if ((fd_shm = shm_open(SHM_NAME_BLOCK, O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR)) == -1) {
        /* En el caso de que la memoria compartida exista llama a link */
        if(errno == EEXIST) return link_shared_block_info();
        perror("shm_open");
        return NULL;
    }

    /* Resize of the memory segment. */
    if (ftruncate(fd_shm, sizeof(shared_block_info)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME_BLOCK);
        return NULL;
    }

    /* Mapping of the memory segment. */
    sbi = mmap(NULL, sizeof(shared_block_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (sbi == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME_BLOCK);
        return NULL;
    }

    /* Inicializamos el semáforo de la memoria compartida */
    if (sem_init(&sbi->mutex, 1, 1) == -1) {
        perror("sem_init");

        munmap(sbi, sizeof(shared_block_info));
        shm_unlink(SHM_NAME_BLOCK);

        return NULL;
    }

    /* Inicializamos variables */
    while(sem_wait(&sbi->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            close_shared_block_info(sbi);
            return NULL;
        }
    }
    sbi->num_miners = 1;

    /* Inicializamos el target a un número aleatorio */
    sbi->target = rand () % (1000000-1+1) + 1;

    sem_post(&sbi->mutex);

    return sbi;
}

shared_block_info *link_shared_block_info() {
    shared_block_info *sbi = NULL;
    int fd_shm;

    /* Open of the shared memory. */
    if ((fd_shm = shm_open(SHM_NAME_BLOCK, O_RDWR, 0)) == -1) {
        perror("shm_open");
        return NULL;
    }

    /* Mapping of the memory segment. */
    sbi = mmap(NULL, sizeof(shared_block_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (sbi == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    /* Inicializamos variables */
    while(sem_wait(&sbi->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            close_shared_block_info(sbi);
            return NULL;
        }
    }
    sbi->num_miners += 1;
    sem_post(&sbi->mutex);

    return sbi;
}

int close_shared_block_info(shared_block_info *sbi) {
    short bool_last_miner = 0;
    if (sbi == NULL) return -1;

    /* Inicializamos variables */
    while(sem_wait(&sbi->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            close_shared_block_info(sbi);
            return -1;
        }
    }
    sbi->num_miners -= 1;

    if (sbi->num_miners == 0)
        bool_last_miner = 1;

    sem_post(&sbi->mutex);

    /* Si somos el último minero en cerrar la memoria compartida */
    if (bool_last_miner == 1) {
        sem_destroy(&sbi->mutex);
        munmap(sbi, sizeof(shared_block_info));
        shm_unlink(SHM_NAME_BLOCK);
    }

    return 0;
}


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
