/**
 * @file monitor.h
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se definen las cabeceras
 * de las funciones usadas por el monitor.
 * @version 0.1
 * @date 2021-05-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#include "net.h"
#include "block.h"
#include "trabajador.h"

#define MQ_NAME "/cola"
#define BUFFER_SIZE 10

typedef struct {
    Block block;
} Mensaje;

#endif