#include <stdio.h>
#include "fifo-queue.h"

#define COUNT 100

fifo_queue_gestor_t gestor;
int queue[N];

void *sender(void *arg)
{
    int index, i;

    for (i = 0; i < COUNT; i++)
    {
        index = get_next_empty_item(&gestor);
        assert(index >= 0);
        queue[index] = i;
        add_full_item(&gestor, index);
    }

    return 0;
}

void *receiveer()
{
    int index, data, i;

    for (i = 0; i < COUNT; i++)
    {
        index = get_next_full_item(&gestor);
        assert(index >= 0);
        data = queue[index];
        assert(data >= 0);
        splice_full_item(&gestor, index);
    }

    return 0;
}

int main(void)
{
    pthread_attr_t attr;
    pthread_t st, rt;

    init_queue(&gestor);
    pthread_attr_init(&attr);

    pthread_create(&st, &attr, sender, NULL);
    pthread_create(&rt, &attr, receiveer, NULL);

    pthread_join(st, NULL);
    pthread_join(rt, NULL);

    pthread_attr_destroy(&attr);

    printf("FIFO-QUEUE-TERMINATED\n");

    return 0;
}