/**
 * @file miner.h
 * @author Kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se definen los prototipos de
 * las funciones usadas por los mineros
 * @version 0.2 - Implementación bloques
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "trabajador.h"
#include "block.h"

#define OK 0
#define MAX_WORKERS 10

#define SHM_NAME_NET "/netdata"
#define SHM_NAME_BLOCK "/block"

#define MAX_MINERS 200

typedef struct _NetData {
    pid_t miners_pid[MAX_MINERS];
    char voting_pool[MAX_MINERS];
    int last_miner;
    int total_miners;
    pid_t monitor_pid;
    pid_t last_winner;
} NetData;


void print_blocks(Block * plast_block, int num_wallets);
