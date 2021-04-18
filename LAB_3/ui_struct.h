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

#include <time.h>

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INPUTMAXSIZE 128
#define BUFFSIZE 5
#define SHM_NAME "/shm_example"
#define MQ_NAME "/cola"

typedef struct {
    char buffer[BUFFSIZE];
    int post_pos;
    int get_pos;
    sem_t sem_empty;
    sem_t sem_fill;
    sem_t sem_mutex;
} ui_struct;

typedef struct {
    char message[INPUTMAXSIZE];
} mensaje;

#endif

