/**
 * @file conc_cycle.c
 * @author Kevin de la Coba Malam
 *         Marcos Aarón Bernuy
 * @brief Archivo donde se define el comportamiento de un programa que:
 *      1. Crea un proceso y este crea otro, ese otro crea otro... así hasta un número determinado.
 *      2. Estos procesos se comunican por señales, el padre envía una señal SIGUSR1
 *      al hijo, el hijo a su hijo... hasta que lleguemos al último proceso, este se
 *      la envía al padre. Los procesos se quedarán en espera no activa hasta que el 
 *      padre envíe de nuevo la señal.
 *      3. Cuando el padre reciba SIGINT, este enviara SIGTERM a los hijos y estos
 *      a sus hijos.
 *      4. Los hijos ignoran todas las señales exceptuando SIGTERM y SIGUSR1, el padre
 *      ignora todas las señales excepto SIGINT y SIGUSR1. Para ignorar dichas señales
 *      se crean máscaras.
 *      5. En cuanto a la sincronización se usan semáforos de forma que en cuanto un
 *      hijo termine de escribir, el siguiente escribe.
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

/* Uso de semáforos */
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#define SEM_NAME_1 "/sem_conc_cycle_1"
#define SEM_NAME_2 "/sem_conc_cycle_2"

/* Flag para determinar si se han recibido diferentes señales */
short sigusr1_received = 0;
short sigterm_received = 0;
short sigint_received = 0;


/**
 * @brief Manejador de la señal SIGALARM
 * Cuando se recibe la señal alarm, el comportamiento 
 * es el mismo al de la señal sigint, por lo que modificamos
 * el valor de singint
 * @param sig Señal
 */
void manejador_SIGALRM(int sig) {
    printf("Alarm recibido.\n");
    sigint_received = 1;
}


/**
 * @brief Función destinada a manejar la señal SIGUSR1.
 * 
 * @param sig Señal recibida
 */
void manejador_SIGUSR(int sig) {
    sigusr1_received = 1;
}


/**
 * @brief Manejador para que marcar que se ha recibido
 * la señal SIGINT.
 * 
 * @param sig Señal recibida
 */
void manejador_SIGINT(int sig) {
    sigint_received = 1;
}


/**
 * @brief Manejador para la señal SIGTERM.
 * 
 * @param sig Señal recibida 
 */
void manejador_SIGTERM(int sig) {
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
        /* Enviamos a nuestro hijo la señal */
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
    sigset_t hijo_mask, padre_mask, old_set;

    /* Semáforos para la ejecución */
    sem_t *sem1 = NULL, *sem2 = NULL;

    /* Estructuras para capturar señales */
    struct sigaction act_SIGUSR1;
    struct sigaction act_SIGINT;
    struct sigaction act_SIGTERM;
    struct sigaction act_SIGALRM;

    if (args < 2) {
        printf("Introduzca el número de procesos.\n./conc_cycle <número de procesos>\n");
        exit(EXIT_FAILURE);
    } else if (atoi(argv[1]) <= 1) {
        printf("Introduzca un número mayor que 1.\n");
        exit(EXIT_FAILURE);
    } else {
        /* Definimos el número de procesos */
        num_proc = atoi(argv[1]);
    }

    /* Iniciamos el temporizador */
    //alarm(10);
    
    /* Inicializamos los semáforos */
    if ((sem1 = sem_open(SEM_NAME_1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		if (errno == EACCES || errno == EEXIST) {
            sem_unlink(SEM_NAME_1);
        }
        perror("sem_open");
		exit(EXIT_FAILURE);
	}
    if ((sem2 = sem_open(SEM_NAME_2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		if (errno == EACCES || errno == EEXIST) {
            sem_unlink(SEM_NAME_1);
            sem_unlink(SEM_NAME_2);
        }
        perror("sem_open");
		exit(EXIT_FAILURE);
	}

    /* Creamos los manejadores */
    act_SIGUSR1.sa_handler = manejador_SIGUSR;
    act_SIGINT.sa_handler = manejador_SIGINT;
    act_SIGTERM.sa_handler = manejador_SIGTERM;
    act_SIGALRM.sa_handler = manejador_SIGALRM;

    /* Inicializando las estructuras sigaction */
    sigemptyset(&(act_SIGUSR1.sa_mask));
    sigemptyset(&(act_SIGINT.sa_mask));
    sigemptyset(&(act_SIGTERM.sa_mask));
    sigemptyset(&(act_SIGALRM.sa_mask));

    act_SIGUSR1.sa_flags = 0;
    act_SIGINT.sa_flags = 0;
    act_SIGTERM.sa_flags = 0;
    act_SIGALRM.sa_flags = 0;

    /* Mascaras para el bloqueo de señales */
    sigfillset(&hijo_mask);
    sigdelset(&hijo_mask, SIGUSR1);
    sigdelset(&hijo_mask, SIGTERM);
    
    sigfillset(&padre_mask);
    sigdelset(&padre_mask, SIGUSR1);
    sigdelset(&padre_mask, SIGINT);
    sigdelset(&padre_mask, SIGALRM);

    pid_P1 = getpid();

    /* Bucle para crear procesos */
    for(int i = 0; i < num_proc-1; i++){
        pid_hijo = fork();
        if (pid_hijo < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if(pid_hijo != 0) break;
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

        if(sigaction(SIGALRM, &act_SIGALRM, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        /* Linkeamos la mascara para bloquear señales */
        if (sigprocmask(SIG_BLOCK, &padre_mask, &old_set) == -1) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }
    }

    /* El padre inicia el ciclo */
    if (pid_P1 == getpid()) {
        while(sem_wait(sem1) == -1) {
            if (errno != EINTR) {
                perror("sem_wait");
                sem_close(sem1);
                sem_close(sem2);
                sem_unlink(SEM_NAME_1);
                sem_unlink(SEM_NAME_2);
                exit(EXIT_FAILURE);
            }
        }
        if (send_signal(pid_hijo, pid_P1, SIGUSR1) == -1) {
            exit(EXIT_FAILURE);
        }

        printf("Número de ciclo: %d, PID hijo: %jd, PID: %jd\n", ciclo, (intmax_t)pid_hijo, (intmax_t)getpid());
        ciclo++;
        sem_post(sem2);
        sem_post(sem1);
    } else if (pid_hijo == 0) {
        /* Cuando el último hijo se haya creado,
        dejamos al padre iniciar el ciclo */
        sem_post(sem1);
    }

    while(1) {
        /* Espera inactiva de la señal, el padre espera sus señales y el hijo otras */
        if (pid_P1 != getpid()) sigsuspend(&hijo_mask);
        else sigsuspend(&padre_mask);

        /* SIGINT */
        if (sigint_received == 1) {
            /* Si el padre recibe la señal SIGINT, envía la señal SIGTERM a los hijos*/
            if (send_signal(pid_hijo, pid_P1, SIGTERM)  == -1) {
                exit(EXIT_FAILURE);
            }

            /* ¿Puedo hacer print? Cuando el semaforo le deje lo hará */
            while(sem_wait(sem2) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    sem_close(sem1);
                    sem_close(sem2);
                    if (pid_P1 == getpid()) {
                        sem_unlink(SEM_NAME_1);
                        sem_unlink(SEM_NAME_2);
                    }
                    exit(EXIT_FAILURE);
                }
            }
            printf("\nMatando hijos, acabando con la ejecución del ciclo. PID: %jd\n", (intmax_t)getpid());
            sem_post(sem2);

            waitpid(pid_hijo, NULL, WEXITED);

            /* El padre hace unlink de los semáforos */
            sem_close(sem1);
            sem_close(sem2);
            sem_unlink(SEM_NAME_1);
            sem_unlink(SEM_NAME_2);
            exit(EXIT_SUCCESS);
            
        } /* SIGTERM */
        else if (sigterm_received == 1) {
            /* Enviamos la señal SIGTERM a nuestro hijo,
            si no tiene hijo, no envía */
            if (pid_hijo != 0) {
                if (send_signal(pid_hijo, pid_P1, SIGTERM)  == -1) {
                    exit(EXIT_FAILURE);
                }
            }

            while(sem_wait(sem2) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    sem_close(sem1);
                    sem_close(sem2);
                    if (pid_P1 == getpid()) {
                        sem_unlink(SEM_NAME_1);
                        sem_unlink(SEM_NAME_2);
                    }
                    exit(EXIT_FAILURE);
                }
            }
            printf("Acabando con la ejecución de PID: %jd\n", (intmax_t)getpid());
            sem_post(sem2);
            /* Cada padre espera a su hijo excepto el último hijo */
            if (pid_hijo != 0) waitpid(pid_hijo, NULL, WEXITED);
            sem_close(sem1);
            sem_close(sem2);
            exit(EXIT_SUCCESS);

        } /* SIGUSR1 */
        else if (sigusr1_received == 1) {
            /* ¿Puedo enviar la señal? Si el semáforo le deja lo hará */
            while(sem_wait(sem1) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    sem_close(sem1);
                    sem_close(sem2);
                    if (pid_P1 == getpid()) {
                        sem_unlink(SEM_NAME_1);
                        sem_unlink(SEM_NAME_2);
                    }
                    exit(EXIT_FAILURE);
                }
            }
            if (send_signal(pid_hijo, pid_P1, SIGUSR1) == -1) {
                exit(EXIT_FAILURE);
            }

            while(sem_wait(sem2) == -1) {
                if (errno != EINTR) {
                    perror("sem_wait");
                    sem_close(sem1);
                    sem_close(sem2);
                    if (pid_P1 == getpid()) {
                        sem_unlink(SEM_NAME_1);
                        sem_unlink(SEM_NAME_2);
                    }
                    exit(EXIT_FAILURE);
                }
            }
            printf("Número de ciclo: %d, PID: %jd\n", ciclo, (intmax_t)getpid());
            sem_post(sem2);
            ciclo++;
            sigusr1_received = 0;
            sem_post(sem1);
        }
    }
}