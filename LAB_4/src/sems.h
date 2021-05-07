/**
 * @file sems.h
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se definen los prototipos 
 * de funciónes para manejar los semáforos que garantizan
 * un acceso concurrente a memoria compartida correcto.
 * @version 0.1 - Votación y concurrencia. 
 * @date 2021-05-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef SEMS_H
#define SEMS_H

#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define SHM_SEMS "/sems"

typedef struct {
    int total_miners;
    int blocked_loosers;
    sem_t net_mutex;
    sem_t block_mutex;
    sem_t mutex;
    sem_t vote;
    sem_t count_votes;
    sem_t update_blocks;
    sem_t update_target;
    sem_t finish;
} Sems;

/**
 * @brief Función que inicializa en memoria compartida varios semáforos.
 * 
 * @return Sems* Region de memoria compartida con semáforos.
 */
Sems *sems_ini();

/**
 * @brief Función que obtiene una zona de memoria compartida ya creada.
 * 
 * @return Sems* Region de memoria compartida.
 */
Sems *link_shared_sems();

/**
 * @brief Función para hacer down a un semáforo.
 * 
 * @param s Semáforo al que hacer down.
 * @return int 0 OK, -1 ERR.
 */
int sem_down(sem_t *s);

/**
 * @brief Función para hacer un up a un semáforo.
 * 
 * @param s Semáforo al que hacer up.
 * @return int 0 OK, -1 ERR.
 */
int sem_up(sem_t *s);

/**
 * @brief Función para cerrar la memoria compartida.
 * 
 * @param sems Memoria que cerrar.
 */
void close_sems(Sems *sems);

#endif