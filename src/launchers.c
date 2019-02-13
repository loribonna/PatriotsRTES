#include "launchers.h"
#include <time.h>
#include <math.h>

atk_gestor_t atk_gestor;

/**
 * INITIALZATIONS
 */

static void init_queue(fifo_queue_gestor_t *gestor)
{
    int i;

    gestor->freeIndex = 0;
    gestor->headIndex = gestor->tailIndex = -1;
    for (i = 0; i < N - 1; i++)
    {
        gestor->next[i] = i + 1;
    }
    gestor->next[N - 1] = -1;

    sem_init(&(gestor->mutex), 0, 1);

    sem_init(&(gestor->read_sem).s, 0, 0);
    gestor->read_sem.c = 0;

    sem_init(&(gestor->write_sem.s), 0, 0);
    gestor->write_sem.c = 0;
}

static void init_atk_launcher()
{
    init_queue(&atk_gestor.gestor);
}

void init_launchers()
{
    init_atk_launcher();
}

/**
 * QUEUE MANAGEMENT
 */

void update_queue_head(fifo_queue_gestor_t *gestor, int index)
{
    assert(index >= 0 && gestor->read_sem.c > 0);

    gestor->next[index] = -1;
    if (gestor->headIndex == -1)
    {
        gestor->headIndex = index;
    }
    else
    {
        gestor->next[gestor->tailIndex] = index;
    }
    gestor->tailIndex = index;
}

void request_atk_launch(fifo_queue_gestor_t *gestor)
{
    int index;

    sem_wait(&(gestor->mutex));

    if (gestor->freeIndex == -1 || !gestor->read_sem.c)
    {
        index = -1;
        sem_post(&(gestor->mutex));
    }
    else
    {
        index = gestor->freeIndex;
        gestor->freeIndex = gestor->next[gestor->freeIndex];
        update_queue_head(gestor, index);
        sem_post(&(gestor->read_sem).s);
    }
}

static int get_next_index(fifo_queue_gestor_t *gestor)
{
    int index;

    sem_wait(&(gestor->mutex));

    if (gestor->headIndex == -1)
    {
        gestor->read_sem.c++;
        sem_post(&(gestor->mutex));
        sem_wait(&(gestor->read_sem.s));
        gestor->read_sem.c--;
    }

    assert(gestor->headIndex >= 0);

    index = gestor->headIndex;
    gestor->headIndex = gestor->next[gestor->headIndex];

    sem_post(&(gestor->mutex));

    return index;
}

static void splice_index(fifo_queue_gestor_t *gestor, int index)
{
    sem_wait(&(gestor->mutex));

    assert(index >= 0);

    gestor->next[index] = gestor->freeIndex;
    gestor->freeIndex = index;

    sem_post(&(gestor->mutex));
}

/**
 * MISSILE FUNCTIONS
 */

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

static int is_deleted_missile(missile_t *missile)
{
    int deleted;

    sem_wait(&missile->mutex);

    deleted = missile->deleted;

    sem_post(&missile->mutex);

    return deleted;
}

static void move_missile(missile_t *missile, float deltatime)
{
    float dx, dy, angle_rad;

    sem_wait(&missile->mutex);

    if (missile->deleted)
    {
        sem_post(&missile->mutex);
        return;
    }

    angle_rad = missile->angle * (M_PI / 180);

    dx = missile->speed * cos(angle_rad);
    dy = missile->speed * sin(angle_rad);

    missile->partial_x += dx * deltatime;
    missile->partial_y += dy * deltatime;

    missile->x = (int)missile->partial_x;
    missile->y = (int)missile->partial_y;

    sem_post(&missile->mutex);
}

static void init_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), ATK_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), ATK_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    params->arg = arg;
}

/**
 * ATTACK THREADS
 */

void clear_atk_missile(missile_t *missile)
{
    sem_wait(&missile->mutex);

    if (!missile->cleared)
    {
        fprintf(stderr, "DELETING Missile %i\n", missile->index);

        missile->cleared = 1;
        missile->speed = 0;

        splice_index(&atk_gestor.gestor, missile->index);
    }

    sem_post(&missile->mutex);
}

ptask atk_thread(void)
{
    missile_t *self;
    float task_index, deltatime, collided, oldx, oldy;

    collided = 0;
    task_index = ptask_get_index();
    deltatime = get_deltatime(task_index, MILLI);
    self = ptask_get_argument();

    fprintf(stderr, "ATK missile %i: position %i %i\n",
            self->index, self->x, self->y);

    while (!is_deleted_missile(self) && !collided)
    {
        oldx = self->x;
        oldy = self->y;
        move_missile(self, deltatime);

        collided = update_missile_env(self, oldx, oldy);

        ptask_wait_for_period();
    }

    clear_atk_missile(self);
}

static int launch_atk_thread(missile_t *missile)
{
    tpars params;

    init_params(&params, missile);

    return ptask_create_param(atk_thread, &params);
}

static void atk_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = ATK_SLEEP_DELAY;
    nanosleep(&t, NULL);
    fprintf(stderr, "WAKEN\n");
}

static void set_random_start(missile_t *missile)
{
    missile->partial_x = 100; // rand() % XWIN;
    missile->partial_y = WALL_THICKNESS + MISSILE_RADIUS + 1;

    missile->speed = frand(1, MAX_SPEED);
    missile->angle = 90; //frand(MAX_ANGLE, 180 - MAX_ANGLE);

    missile->x = (int)missile->partial_x;
    missile->y = (int)missile->partial_y;
}

static void init_empty_missile(missile_t *missile)
{
    sem_init(&missile->mutex, 0, 1);
    missile->deleted = missile->cleared = 0;
    missile->speed = missile->angle = 0;
    missile->x = missile->y = 0;
    missile->index = -1;
}

static void init_atk_missile(missile_t *missile, int index)
{
    init_empty_missile(missile);
    missile->index = index;
    missile->missile_type = ATTACKER;
    set_random_start(missile);
}

static void launch_atk_missile(int index)
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
        index = get_next_index(&atk_gestor.gestor);
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
    missile_t *missile;

    missile = &(atk_gestor.queue[index]);
    missile->deleted = 1;
}

//TODO
void delete_def_missile(int index)
{
}