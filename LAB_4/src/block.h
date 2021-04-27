/**
 * @file block.h
 * @author kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se definen los prototipos
 * de las funciones usadas para manejar bloques.
 * @version 0.1 - Implementación de bloques.
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef BLOCK_H
#define BLOCK_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_MINERS 200
#define PRIME 99997669
#define BIG_X 435679812
#define BIG_Y 100001819

typedef struct _Block {
    int wallets[MAX_MINERS];
    long int target;
    long int solution;
    int id;
    int is_valid;
    struct _Block *next;
    struct _Block *prev;
} Block;

/**
 * @brief Función para reserver memoria de un bloque.
 * 
 * @return Block* Bloque creado.
 */
Block *block_ini();

/**
 * @brief Función para inicializar el bloque.
 * El target del nuevo bloque es la solución del anterior.
 * 
 * @param prev Bloque anterior (o perteneciente a la blockchain).
 * @param block Bloque a inicializar.
 * @return int -1 ERR, 0 OK.
 */
int block_set(Block *prev, Block *block);

/**
 * @brief Función para destruir un bloque.
 * 
 * @param block Bloque a destruir.
 */
void block_destroy(Block *block);

/**
 * @brief Función para destruir la blockchain entera.
 * 
 * @param block Bloque perteneciente a la blockchain.
 */
void block_destroy_blockchain(Block *block);

#endif