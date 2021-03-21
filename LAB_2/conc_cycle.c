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

/* Para hacer conversión a intmax_t */
#include <stdint.h>

/* Uso de fork, getpid */
#include <sys/types.h>
#include <unistd.h>

/* Uso de señales */
#include <signal.h>

/* Manejo de errores */
#include <errno.h>

/* Espera a los hijos */
#include <sys/wait.h>

/* Flag para determinar si se han recibido diferentes señales */
short sigusr1_received = 0;
short sigterm_received = 0;
short sigint_received = 0;


/**
 * @brief Función destinada a manejar la señal SIGUSR1.
 * 
 * @param sig Señal recibida
 */
void manejador_SIGUSR(int sig) {
    printf("hola_ sigusr1\n");
    sigusr1_received = 1;
}


/**
 * @brief Manejador para que marcar que se ha recibido
 * la señal SIGINT.
 * 
 * @param sig Señal recibida
 */
void manejador_SIGINT(int sig) {
    printf("hola_ sigint\n");
    sigint_received = 1;
}


/**
 * @brief Manejador para la señal SIGTERM.
 * 
 * @param sig Señal recibida 
 */
void manejador_SIGTERM(int sig) {
    printf("hola_ sigterm\n");
    sigterm_received = 1;
}


/**
 * @brief Función para enviar al hijo la señal
 * 
 * @param hijo PID del hijo
 * @param pid_P1 PID del primer proceso
 * @param signal Señal a enviar
 * @return int  0 OK, -1 ERR
 */
int send_signal(int hijo, int pid_P1, int signal) {
    if (hijo != 0) { // 
        /* Enviamos a nuestro hijo la señal SIGUSR1 */
        if (kill(hijo, signal) == -1) {
            perror("kill");
            return -1;
        }
    } else {
        /* Si no tenemos hijo, se lo enviamos a P1 */
        if (kill(pid_P1, signal) == -1) {
            perror("kill");
            return -1;
        }
    }
    return 0;
}


int main(int args, char* argv[]) {
    int num_proc = 0, ciclo = 1;
    pid_t pid_hijo = 0, pid_P1 = 0;
    sigset_t hijo_mask, old_set;

    /* Estructuras para capturar señales */
    struct sigaction act_SIGUSR1;
    struct sigaction act_SIGINT;
    struct sigaction act_SIGTERM;

    if (args < 2) {
        printf("Introduzca el número de procesos.\n./conc_cycle <número de procesos>\n");
        return -1;
    } else {
        /* Definimos el número de procesos */
        num_proc = atoi(argv[1]);
    }

    /* Creamos los manejadores */
    act_SIGUSR1.sa_handler = manejador_SIGUSR;
    act_SIGINT.sa_handler = manejador_SIGINT;
    act_SIGTERM.sa_handler = manejador_SIGUSR;

    /* Inicializando las mascaras */
    sigemptyset(&(act_SIGUSR1.sa_mask));
    sigemptyset(&(act_SIGINT.sa_mask));
    sigemptyset(&(act_SIGTERM.sa_mask));

    act_SIGUSR1.sa_flags = 0;
    act_SIGINT.sa_flags = 0;
    act_SIGTERM.sa_flags = 0;

    /* Mascaras para el bloqueo de señales */
    sigemptyset(&hijo_mask);
    sigaddset(&hijo_mask, SIGINT);

    pid_P1 = getpid();

    /* Bucle para crear procesos */
    for(int i = 0; i < num_proc-1; i++){
        pid_hijo = fork();
        if(pid_hijo != 0) break;
    }

    /* Linkeamos la estructura sigaction a los procesos hijos */
    if(pid_P1 != getpid()){
        if(sigaction(SIGUSR1, &act_SIGUSR1, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        if(sigaction(SIGTERM, &act_SIGTERM, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        /* Linkeamos la mascara para bloquear señales */
        if (sigprocmask(SIG_BLOCK, &hijo_mask, &old_set) == -1) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }
    }
    /* Linkeamos la estructura sigaction al proceso antecesor */
    else {
        if(sigaction(SIGUSR1, &act_SIGUSR1, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        if(sigaction(SIGINT, &act_SIGINT, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
    }

    /* El padre inicia el ciclo */
    if (pid_P1 == getpid()) {
        sleep(1);
        send_signal(pid_hijo, pid_P1, SIGUSR1);
        printf("Número de ciclo: %d, PID hijo: %jd, PID: %jd\n", ciclo, (intmax_t)pid_hijo, (intmax_t)getpid());
        ciclo++;
    }

    while(1) {
        /* Espera inactiva de la señal */
        pause();
        sleep(1);
        if (sigusr1_received == 1) {
            send_signal(pid_hijo, pid_P1, SIGUSR1);
            printf("Número de ciclo: %d, PID hijo: %jd, PID: %jd\n", ciclo, (intmax_t)pid_hijo, (intmax_t)getpid());
            ciclo++;
            sigusr1_received = 0;
        }
        else if (sigint_received == 1){
            printf("SIGINT recibido, acabando con la ejecución del ciclo. PID: %jd\n", (intmax_t)getpid());
            /* Si el padre recibe la señal SIGINT, envía la señal SIGTERM a los hijos*/
            send_signal(pid_hijo, pid_P1, SIGTERM);
            printf("SIGINT recibido, acabando con la ejecución del ciclo. PID: %jd\n", (intmax_t)getpid());
            waitpid(pid_hijo, NULL, WEXITED);
            exit(EXIT_SUCCESS);
        } else if (sigterm_received == 1) {
            /* Enviamos la señal SIGTERM a nuestro hijo,
            si no tiene hijo, no envía */
            if (pid_hijo != 0) send_signal(pid_hijo, pid_P1, SIGTERM);
            printf("Acabando con la ejecución de PID: %jd\n", (intmax_t)getpid());

            /* Cada hijo espera a su hijo excepto el último hijo */
            if (pid_hijo != 0) waitpid(pid_hijo, NULL, WEXITED);
            exit(EXIT_SUCCESS);
        }
    }
}