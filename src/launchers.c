#include "launchers.h"
#include <time.h>
#include <math.h>

atk_gestor_t atk_gestor;

static void init_atk_launcher()
{
    init_queue(&atk_gestor.gestor);
}

void init_launchers() {
    init_atk_launcher();
}

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

int is_queue_full(fifo_queue_gestor_t *gestor)
{
    int ret;

    sem_wait(&(gestor->mutex_insert));

    ret = gestor->freeIndex == -1;

    sem_post(&(gestor->mutex_insert));

    return ret;
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

    assert(gestor->freeIndex >= 0);

    index = gestor->freeIndex;
    gestor->freeIndex = gestor->next[gestor->freeIndex];

    sem_post(&(gestor->mutex_insert));

    return index;
}

void add_full_item(fifo_queue_gestor_t *gestor, int index)
{
    sem_wait(&(gestor->mutex_remove));

    assert(index >= 0);

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

    assert(gestor->headIndex >= 0);

    index = gestor->headIndex;
    gestor->headIndex = gestor->next[gestor->headIndex];

    sem_post(&(gestor->mutex_remove));

    return index;
}

void splice_full_item(fifo_queue_gestor_t *gestor, int index)
{
    sem_wait(&(gestor->mutex_insert));

    assert(index >= 0);

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

void init_missile(missile_t *missile)
{
    sem_init(&missile->deleted, 0, 0);
    missile->speed = missile->angle = 0;
    missile->x = missile->y = 0;
    missile->index = -1;
}

int draw_missile(BITMAP *buffer, int x, int y, missile_type_t type)
{
    int color;

    if (!check_borders(x, y))
    {
        return -1;
    }

    if (type == ATTACKER)
    {
        color = ATTACKER_COLOR;
    }
    else
    {
        color = DEFENDER_COLOR;
    }

    circlefill(buffer, x, y, MISSILE_RADIUS, color);

    return 0;
}

void delete_missile(missile_t *missile)
{
    sem_post(&missile->deleted);
}

int is_deleted(missile_t *missile)
{
    int sval;

    sem_getvalue(&missile->deleted, &sval);

    return sval;
}

void move_missile(missile_t *missile, float deltatime)
{
    float dx, dy, angle_rad;

    angle_rad = missile->angle * (M_PI / 180);

    dx = missile->speed * cos(angle_rad);
    dy = missile->speed * sin(angle_rad);

    missile->x += dx * deltatime;
    missile->y += dy * deltatime;
}

void print_missile(missile_t *missile)
{
    fprintf(stderr, "i: %i, x: %f, y: %f, angle: %f\n",
            missile->index, missile->x, missile->y, missile->angle);
}

ptask atk_thread(void)
{
    missile_t *self;
    float task_index, deltatime, collided;

    collided = 0;
    task_index = ptask_get_index();
    deltatime = get_deltatime(task_index, MILLI);
    self = ptask_get_argument();

    fprintf(stderr, "ATK missile %i: position %f %f\n", self->index, self->x, self->y);

    while (!is_deleted(self) && !collided)
    {
        collided = update_missile_position(self, deltatime);

        ptask_wait_for_period();
    }

    delete_atk_missile(self->index);
}

void init_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), ATK_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), ATK_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    params->arg = arg;
}

int launch_atk_thread(missile_t *missile)
{
    tpars params;

    init_params(&params, missile);

    return ptask_create_param(atk_thread, &params);
}

void atk_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = ATK_SLEEP_DELAY;
    nanosleep(&t, NULL);
    fprintf(stderr, "WAKEN\n");
}

void set_random_start(missile_t *missile)
{
    missile->x = rand() % XWIN;
    missile->y = WALL_THICKNESS + MISSILE_RADIUS + 1;

    missile->speed = frand(1, MAX_SPEED);
    missile->angle = frand(MAX_ANGLE, 180 - MAX_ANGLE);
}

void init_atk_missile(missile_t *missile, int index)
{
    init_missile(missile);
    missile->index = index;
    missile->missile_type = ATTACKER;
    set_random_start(missile);
}

void launch_atk_missile(int index)
{
    missile_t *missile;
    int thread;

    missile = &(atk_gestor.queue[index]);
    init_atk_missile(missile, index);

    thread = launch_atk_thread(missile);
    assert(thread >= 0);
}

ptask atk_launcher()
{
    int index;

    while (1)
    {
        index = get_next_full_item(&atk_gestor.gestor);
        fprintf(stderr, "Launching ATK missile index: %i\n", index);
        launch_atk_missile(index);
        atk_wait();
    }
}

void launch_atk_launcher()
{
    int task;

    task = ptask_create_prio(atk_launcher,
                             ATK_LAUNCHER_PERIOD,
                             ATK_LAUNCHER_PRIO,
                             NOW);

    assert(task >= 0);

    fprintf(stderr, "Created ATK launcher\n");
}

void delete_atk_missile(int index)
{
    delete_missile(&(atk_gestor.queue[index]));
    splice_full_item(&atk_gestor.gestor, index);
}

//TODO
void delete_def_missile(int index) {

}