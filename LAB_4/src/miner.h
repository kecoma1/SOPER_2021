/**
 * @file miner.h
 * @author Kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se definen los prototipos de
 * las funciones usadas por los mineros
 * @version 0.1 - Minero paralelo.
 *          0.2 - Implementación bloques.
 *          0.3 - Memoria compartida bloques.
 *          0.4 - Red de mineros.
 *          0.5 - Votación y concurrencia.
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
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <signal.h>

#include "trabajador.h"
#include "block.h"
#include "net.h"
#include "sems.h"

#define OK 0
#define MAX_WORKERS 10
#define MAX_MINERS 200

typedef struct {
    NetData *nd;
    shared_block_info *sbi;
    Sems *sems;
    worker_struct *threads_info;
    Block *block;
    sem_t mutex;
} Miner_data;
