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

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <mqueue.h>
#include <time.h>

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