#include "fifo-queue.h"

void init_queue(fifo_queue_gestor_t *gestor)
{
    gestor->freeIndex = 0;
    gestor->tailIndex = -1;
    gestor->headIndex = -1;
    for (int i = 0; i < N - 1; i++)
    {
        gestor->next[i] = i + 1;
    }
    gestor->next[N - 1] = -1;

    sem_init(&(gestor->mutex_insert), 0, 1);
    sem_init(&(gestor->mutex_remove), 0, 1);

    sem_init(&(gestor->empty_sem).s, 0, 0);
    sem_init(&(gestor->full_sem).s, 0, 0);
    gestor->empty_sem.c = 0;
    gestor->full_sem.c = 0;
}

int get_next_empty_item(fifo_queue_gestor_t *gestor)
{
    int index;

    sem_wait(&(gestor->mutex_insert));

    if (gestor->empty_sem.c || gestor->freeIndex == -1)
    {
        gestor->empty_sem.c++;
        sem_post(&(gestor->mutex_insert));
        sem_wait(&(gestor->empty_sem).s);
        gestor->empty_sem.c--;
    }

    index = gestor->freeIndex;
    gestor->freeIndex = gestor->next[gestor->freeIndex];

    sem_post(&(gestor->mutex_insert));

    return index;
}

void add_full_item(fifo_queue_gestor_t *gestor, int index)
{
    sem_wait(&(gestor->mutex_remove));

    gestor->next[index] = -1;
    if (gestor->headIndex < 0)
    {
        gestor->headIndex = index;
    }
    else
    {
        gestor->next[gestor->tailIndex] = index;
    }
    gestor->tailIndex = index;

    if (gestor->full_sem.c)
    {
        sem_post(&(gestor->full_sem).s);
    }
    else
    {
        sem_post(&(gestor->mutex_remove));
    }
}

int get_next_full_item(fifo_queue_gestor_t *gestor)
{
    int index;

    sem_wait(&(gestor->mutex_remove));

    if (gestor->headIndex == -1)
    {
        gestor->full_sem.c++;
        sem_post(&(gestor->mutex_remove));
        sem_wait(&gestor->full_sem.s);
        gestor->full_sem.c--;
    }

    index = gestor->headIndex;
    gestor->headIndex = gestor->next[gestor->headIndex];

    sem_post(&(gestor->mutex_remove));

    return index;
}

void splice_full_item(fifo_queue_gestor_t *gestor, int index)
{
    sem_wait(&(gestor->mutex_insert));

    gestor->next[index] = gestor->freeIndex;
    gestor->freeIndex = index;

    if (gestor->empty_sem.c)
    {
        sem_post(&(gestor->empty_sem).s);
    }
    else
    {
        sem_post(&(gestor->mutex_insert));
    }
}