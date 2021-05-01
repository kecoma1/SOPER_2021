/**
 * @file trabajador.h
 * @author  Kevin de la Coba Malam
 *          Marcos Aarón Bernuy
 * @brief Cabecera donde se definen los prototipos de las funciones
 * del trabajador.
 * @version 0.1
 * @date 2021-04-27
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef TRABAJADOR_H
#define TRABAJADOR_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define PRIME 99997669
#define BIG_X 435679812
#define BIG_Y 100001819

/**
 * @brief Estructura que recibe el trabajador.
 * La estructura contiene el índice por donde debe empezar a calcular
 * y el indice por donde debe acabar, también contiene el target que buscamos
 * y un id.
 */
typedef struct {
    int starting_index;
    int ending_index;
    long int target;
    long int solution;
} worker_struct;

/**
 * @brief Función que calcula un resultado dada una entrada.
 * 
 * @param number Entrada.
 * @return long int Resultado.
 */
long int simple_hash(long int number);

/**
 * @brief Función diseñada para que sea ejecutada por un hilo.
 * 
 * @param arg Estructura
 * @return void* NULL
 */
void *work_thread(void *arg);

#endif