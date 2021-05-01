/**
 * @file net.h
 * @author kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se definen los prototipos
 * de las funciones usadas para manejar la memoria
 * compartida (la red).
 * @version 0.1
 * @date 2021-05-01
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef NET_H
#define NET_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_MINERS 200

typedef struct _NetData {
    pid_t miners_pid[MAX_MINERS];
    char voting_pool[MAX_MINERS];
    int last_miner;
    int total_miners;
    pid_t monitor_pid;
    pid_t last_winner;
} NetData;

#endif