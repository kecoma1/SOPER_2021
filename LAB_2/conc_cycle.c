/**
 * @file conc_cycle.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se define el comportamiento de un programa que:
 *      1. Crea un proceso y este crea otro... así hasta un número determinado.
 *      2. Estos procesos se sincronizan por señales, el padre envía una señal SIGUSR1
 *      al hijo, el hijo a su hijo... hasta que lleguemos al último proceso, este se
 *      la envía al padre. Los procesos se quedarán en espera no activa hasta que el 
 *      padre envíe de nuevo la señal.
 *      3. Cuando el padre reciba SIGINT, este enviara SIGTERM a los hijos y estos
 *      a sus hijos.
 * @version 1.0
 * @date 2021-03-16
 * 
 * @copyright Copyright (c) 2021
 * 
 */


/* Salida y entrada estandar */
#include <stdio.h> 

/* Funciones básicas para la ejecución */
#include <stdlib.h> 

/* Uso de fork, getpid */
#include <sys/types.h>
#include <unistd.h>

/* Uso de señales */
#include <signal.h>

/* Manejo de errores */
#include <errno.h>

/* Espera a los hijos */
#include <sys/wait.h>

/* Variables globales para que el 
manejador pueda acceder a estas */
int ciclo = 0;
pid_t pid_hijo = 0, pid = 0;


/**
 * @brief Función destinada a manejar la señal SIGUSR1.
 * 
 * @param sig Señal recibida
 */
void manejador_SIGUSR(int sig) {
    /* Sleep para que sea "visible" la ejecución */
    sleep(1);

    if (pid_hijo == 0) {
        /* Enviamos SIGUSR1 al padre de todos */
        if (kill(pid, SIGUSR1) == -1) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
    } else {
        /* Enviamos SIGUSR1 al hijo */
        if (kill(pid_hijo, SIGUSR1) == -1) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
    }
    ciclo++;
    printf("Ciclo: %d - PID: %d\n", ciclo, getpid());
}


/**
 * @brief Manejador para que cuando el padre reciba
 * SIGINT, este envié a su hijo SIGTERM, y el hijo
 * a su hijo...
 * 
 * @param sig Señal recibida
 */
void manejador_SIGINT(int sig) {
    /* Enviamos SIGTERM al hijo */
    if (kill(pid_hijo, SIGTERM) == -1) perror("kill");

    printf("Padre abandona a sus hijos.\n");

    /* Esperamos al hijo */
    waitpid(pid_hijo, NULL, WEXITED);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Manejador para la señal SIGTERM.
 * Cada vez que un proceso recibe la señal,
 * este libera recursos y termina su ejecución
 * aquí.
 * 
 * @param sig Señal recibida 
 */
void manejador_SIGTERM(int sig) {
    printf("PID %d termina su ejecución.\n", getpid());
    fflush(stdin);

    /* Enviamos SIGTERM al hijo (si tiene) */
    if (pid_hijo != 0) {
        if (kill(pid_hijo, SIGTERM) == -1) perror("kill");
    }

    /* Esperamos a nuestro hijo */
    waitpid(pid_hijo, NULL, WEXITED);
    exit(EXIT_SUCCESS);
}


int main(int args, char* argv[]) {
    int num_proc = 0, i = 0;

    /* Estructuras para capturar señales */
    struct sigaction act_SIGUSR1;
    struct sigaction act_SIGTERM;
    struct sigaction act_SIGINT;

    if (args < 2) {
        printf("Introduzca el número de procesos.\n./conc_cycle <número de procesos>\n");
        return -1;
    } else {
        /* Definimos el número de procesos */
        num_proc = atoi(argv[1]);
    }

    /* Creamos los manjeadores */
    act_SIGUSR1.sa_handler = manejador_SIGUSR;
    act_SIGTERM.sa_handler = manejador_SIGTERM;
    act_SIGINT.sa_handler = manejador_SIGINT;

    sigemptyset(&(act_SIGUSR1.sa_mask));
    sigemptyset(&(act_SIGTERM.sa_mask));
    sigemptyset(&(act_SIGINT.sa_mask));

    act_SIGUSR1.sa_flags = 0;
    act_SIGTERM.sa_flags = 0;
    act_SIGINT.sa_flags = 0;

    /* Obtenemos el pid del padre */
    pid = getpid();

    /* Arbol de procesos */
    for(i = 0; i < num_proc-1; i++) {
        pid_hijo = fork();
        
        /* Ejecución del padre, sale del loop */
        if (pid_hijo != 0) break;
    }

    /* "linkeamos" las señales con los manejadores */
    if (sigaction(SIGUSR1, &act_SIGUSR1, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &act_SIGTERM, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* "Linkeamos" el manejador de SIGINT al padre */
    if (pid == getpid()) {
        if (sigaction(SIGINT, &act_SIGINT, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
    }

    /* Dromimos al proceso para que espere a que sus hijos se creen */
    sleep(1);

    /* Si somos el padre iniciamos el ciclo */
    if (pid == getpid()) {
        if (kill(pid_hijo, SIGUSR1) == -1) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
    }

    /* Bucle para enviar señales */
    while (1) {
        pause();
    }
}