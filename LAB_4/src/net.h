/**
 * @file net.h
 * @author kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Archivo donde se definen los prototipos
 * de las funciones usadas para manejar la memoria
 * compartida (la red).
 * @version 0.1 - Implementación de la red
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
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>

#define MAX_MINERS 200
#define SHM_NAME_NET "/netdata"

typedef struct _NetData {
    pid_t miners_pid[MAX_MINERS];
    char voting_pool[MAX_MINERS];
    int last_miner;
    int total_miners;
    int num_voters;
    pid_t monitor_pid;
    pid_t last_winner;
    sem_t mutex;
    sem_t check_voting;
    sem_t start_next_round;
} NetData;

/**
 * @brief Función para crear una red en memoria compartida.
 * Si ya esta creada se llama a link_net.
 * 
 * @return NetData* Zona de memoria compartida con la Red.
 */
NetData *create_net();

/**
 * @brief Función que obtiene una zona de memoria compartida
 * ya creada.
 * 
 * @return NetData* Zona de memoria compartida con la Red.
 */
NetData *link_shared_net();

/**
 * @brief Función para obtener el Indice donde se almacena nuestro PID.
 * 
 * @param nd NetData donde buscar.
 * @return int Indice.
 */
int net_get_index(NetData *nd);

/**
 * @brief Función para obtener el quorum. Envía 
 * SIGUSR1 a todos los mineros y comprueba que 
 * envíos son correctos para determinar el número 
 * exacto de mineros activos. Se debe haber bajado
 * el mutex de la red antes de llamar a la función.
 *
 * @param nd NetData. 
 * @return int Número de participantes activos.
 */
int get_quorum(NetData *nd);

/**
 * @brief Función que cierra la memoria compartida.
 * 
 * @param nd Memoria que cerrar.
 * @return int 0 OK, 1 ERR
 */
int close_net(NetData *nd);

#endif