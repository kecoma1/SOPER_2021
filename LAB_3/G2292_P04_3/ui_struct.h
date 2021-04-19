/**
 * @file ui_struct.h
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Header donde se definen las estructuras usadas y
 * las librerias necesarias para la ejecución del programa.
 * @version 1.0
 * @date 2021-04-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef STRUCT_UI_H
#define STRUCT_UI_H

/* Uso de shm */
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Uso de cola de mensajes */
#include <mqueue.h>

/* Uso de fork y execl */
#include <unistd.h>

/* Manejo de strings */
#include <string.h>

/* Espera de los procesos */
#include <wait.h>

/* Uso de señales */
#include <signal.h>

/* Uso de semáforos */
#include <semaphore.h>

/* Para recoger el tiempo actual */
#include <time.h>

/* Manejo de errores */
#include <errno.h>

/* Uso de funciones básicas */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define INPUTMAXSIZE 128
#define BUFFSIZE 5
#define SHM_NAME "/shm_example"
#define MQ_NAME_SERVER "/cola_server"
#define MQ_NAME_CLIENT "/cola_client"

/**
 * @brief Estructura donde se encuentran los datos
 * compartidos entre procesos.
 * 
 */
typedef struct {
    char buffer[BUFFSIZE];
    int post_pos;
    int get_pos;
    sem_t sem_empty;
    sem_t sem_fill;
    sem_t sem_mutex;
} ui_struct;

/**
 * @brief Estructura de los mensajes.
 * 
 */
typedef struct {
    char message[INPUTMAXSIZE];
} Mensaje;

#endif