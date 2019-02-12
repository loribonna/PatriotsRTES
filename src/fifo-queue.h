#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

#include <semaphore.h>
#include <pthread.h>
#include "utils.h"

struct private_sem_t
{
    sem_t s;
    int c;
};

typedef struct
{
    int freeIndex, tailIndex, headIndex;
    int next[N];
    sem_t mutex_insert, mutex_remove;
    struct private_sem_t empty_sem;
    struct private_sem_t full_sem;
} fifo_queue_gestor_t;

int is_queue_full(fifo_queue_gestor_t *gestor);

void init_queue(fifo_queue_gestor_t *gestor);

int get_next_empty_item(fifo_queue_gestor_t *gestor);

void add_full_item(fifo_queue_gestor_t *gestor, int index);

int get_next_full_item(fifo_queue_gestor_t *gestor);

void splice_full_item(fifo_queue_gestor_t *gestor, int index);

#endif