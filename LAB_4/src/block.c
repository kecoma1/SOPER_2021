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
    block->id = last_block->id + 1;
    block->next = NULL;
    block->prev = last_block;

    /* Si somos el primer bloque no hay anterior */
    if (last_block != NULL) block->target = last_block->solution;
    else block->target = 1; // TODO RANDOM NUM

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
    
    last_block->next = block;

    return 0;
}

void block_destroy(Block *block) {
    if (block == NULL)
        return;

    free(block);
    block = NULL;    
}

void block_destroy_blockchain(Block *block) {
    Block *aux, *last_block;
    if (block == NULL)
        return;
    
    /* Vamos al último bloque de la blockchain */
    last_block = block;
    while (last_block->next != NULL) {
        last_block = last_block->next;
    }

    /* Borramos la blockchain desde el final */
    while (aux != NULL) {
        aux = last_block->prev;
        block_destroy(last_block);
        last_block = aux;
    }
}
