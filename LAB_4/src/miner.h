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

#include "trabajador.h"
#include "block.h"
#include "net.h"

#define OK 0
#define MAX_WORKERS 10
#define MAX_MINERS 200
