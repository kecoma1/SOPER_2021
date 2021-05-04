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
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define MAX_MINERS 200

#define SHM_NAME_BLOCK "/block"

typedef struct _Block {
    int wallets[MAX_MINERS];
    long int target;
    long int solution;
    int id;
    int is_valid;
    struct _Block *next;
    struct _Block *prev;
} Block;

typedef struct {
    long int target;
    long int solution;
    int id;
    int is_valid;
    int num_miners;
    int wallets[MAX_MINERS];
} shared_block_info;

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
 * @brief Función para copiar la información de un bloque en otro.
 * 
 * @param src Bloque del que copiar.
 * @param dest Bloque al que copiar.
 * @return int 0 OK, -1 ERR.
 */
int block_copy(Block *src, Block *dest);

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

/**
 * @brief Crea memoria compartida para la información del ultimo bloque.
 * 
 * @return shared_block_info* Memoria compartida.
 */
shared_block_info *create_shared_block_info();

/**
 * @brief Función que accede a una memoria compartida ya creada.
 * 
 * @return shared_block_info* Memoria compartida.
 */
shared_block_info *link_shared_block_info();

/**
 * @brief Función que cierra la memoria compartida.
 * 
 * @param fd Descriptor a cerrar
 */
void close_shared_block_info(shared_block_info *sbi);

/**
 * @brief Función para actualizar un bloque local obteniendo
 * los datos de la memoria compartida.
 * 
 * @param sbi Memoria compartida.
 * @param block Bloque local.
 * @return short 0 OK, -1 ERR.
 */
short update_block(shared_block_info *sbi, Block *block);

/**
 * @brief Función para imprimir la blockchain en un archivo.
 * 
 * @param pf Archivo donde imprimir.
 * @param block Bloque en el que imprimir. 
 */
void print_blocks_in_file(FILE *pf, Block * block);

void print_blocks(Block * plast_block, int num_wallets);


#endif