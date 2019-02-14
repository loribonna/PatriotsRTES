#include "launchers.h"
#include <time.h>
#include <math.h>

missile_gestor_t atk_gestor;
missile_gestor_t def_gestor;

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

static void init_empty_missile(missile_t *missile)
{
    sem_init(&missile->mutex, 0, 1);
    missile->deleted = missile->cleared = 0;
    missile->speed = missile->angle = 0;
    missile->x = missile->y = 0;
    missile->target = missile->index = -1;
}

static void init_missiles_gestor(missile_gestor_t *m_gestor)
{
    int i;

    init_queue(&m_gestor->gestor);

    for (i = 0; i < N; i++)
    {
        init_empty_missile(&(m_gestor->queue[i]));
    }
}

static void init_atk_launcher()
{
    init_missiles_gestor(&atk_gestor);
}

static void init_def_launcher()
{
    init_missiles_gestor(&def_gestor);
}

void init_launchers()
{
    init_atk_launcher();
    init_def_launcher();
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

void request_atk_launch()
{
    int index;
    fifo_queue_gestor_t *gestor;

    gestor = &(atk_gestor.gestor);

    sem_wait(&(gestor->mutex));

    if (gestor->freeIndex == -1 || !gestor->read_sem.c)
    {
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

int request_def_launch()
{
    int index;
    fifo_queue_gestor_t *gestor;

    gestor = &(def_gestor.gestor);

    sem_wait(&(gestor->mutex));

    if (gestor->freeIndex == -1 || gestor->write_sem.c)
    {
        gestor->write_sem.c++;
        sem_post(&(gestor->mutex));
        sem_wait(&(gestor->write_sem.s));
        gestor->write_sem.c--;
    }

    index = gestor->freeIndex;
    gestor->freeIndex = gestor->next[gestor->freeIndex];

    sem_post(&(gestor->mutex));

    return index;
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

    if (gestor->write_sem.c)
    {
        sem_post(&gestor->write_sem.s);
    }
    else
    {
        sem_post(&(gestor->mutex));
    }
}

/**
 * MISSILE FUNCTIONS
 */

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

static void init_atk_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), ATK_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), ATK_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    params->arg = arg;
}

void clear_missile(missile_t *missile, fifo_queue_gestor_t *gestor)
{
    sem_wait(&missile->mutex);

    if (!missile->cleared)
    {
        missile->cleared = 1;
        missile->speed = 0;

        splice_index(gestor, missile->index);
    }

    sem_post(&missile->mutex);
}

static void task_missile_movement(missile_t *missile, int task_index)
{
    int oldx, oldy, collided;
    float deltatime;

    collided = 0;
    deltatime = get_deltatime(task_index, MILLI);

    while (!is_deleted_missile(missile) && !collided)
    {
        oldx = missile->x;
        oldy = missile->y;
        move_missile(missile, deltatime);

        collided = update_missile_env(missile, oldx, oldy);

        ptask_wait_for_period();
    }
}

/**
 * ATTACK THREADS
 */

void clear_atk_missile(missile_t *missile)
{
    clear_missile(missile, &atk_gestor.gestor);
}

ptask atk_thread(void)
{
    missile_t *self;
    int task_index;

    task_index = ptask_get_index();
    self = ptask_get_argument();

    task_missile_movement(self, task_index);

    clear_atk_missile(self);
}

static int launch_atk_thread(missile_t *missile)
{
    tpars params;

    init_atk_params(&params, missile);

    return ptask_create_param(atk_thread, &params);
}

static void atk_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = ATK_SLEEP_DELAY;
    nanosleep(&t, NULL);
}

static void set_random_start(missile_t *missile)
{
    missile->partial_x = (int)frand(0, XWIN);
    missile->partial_y = WALL_THICKNESS + MISSILE_RADIUS + 1;

    missile->speed = frand(1, MAX_SPEED);
    missile->angle = frand(MAX_ANGLE, 180 - MAX_ANGLE);

    missile->x = (int)missile->partial_x;
    missile->y = (int)missile->partial_y;
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
        launch_atk_missile(index);
        atk_wait();
        ptask_wait_for_period();
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

/**
 * DEFENDER THREADS
 */

static void def_missile_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = DEF_MISSILE_DELAY;
    nanosleep(&t, NULL);
}

static void def_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = DEF_SLEEP_DELAY;
    nanosleep(&t, NULL);
}

int is_already_tracked(int target)
{
    int i, ret;

    ret = 0;

    sem_wait(&def_gestor.gestor.mutex);

    for (i = 0; i < N; i++)
    {
        ret |= def_gestor.queue[i].target == target;
    }

    sem_post(&def_gestor.gestor.mutex);

    return ret;
}

void clear_def_missile(missile_t *missile)
{
    clear_missile(missile, &def_gestor.gestor);
}

static float get_pos_distance(pos_t *pos_a, pos_t *pos_b)
{
    return sqrt(pow(pos_a->x - pos_b->x, 2) + pow(pos_a->y - pos_b->y, 2));
}

static float get_line_m(pos_t *pos_a, pos_t *pos_b)
{
    if (pos_b->x == pos_a->x)
    {
        return 0;
    }
    return (pos_b->y - pos_a->y) / (pos_b->x - pos_a->x);
}

static float get_line_angle(pos_t *pos_a, pos_t *pos_b)
{
    return atan2((pos_b->y - pos_a->y), (pos_b->x - pos_a->x));
}

static float get_line_b_with_m(float m, pos_t *pos)
{
    return -m * pos->x + pos->y;
}

static float get_x_from_trajectory(trajectory_t *t, float y)
{
    return t->m != 0 ? (y - t->b) / t->m : 0;
}

static float get_expected_position_x(trajectory_t *t, pos_t *current)
{
    float dsa, end_y, dsb;

    dsa = DS_AMOUNT(t->speed);

    do
    {
        end_y = (t->speed / DEF_MISSILE_SPEED) * dsa + current->y;
        dsb = end_y - DEF_MISSILE_START_Y;
        dsa = (dsb + dsa) / 2;
    } while (dsb - dsa > EPSILON);

    return get_x_from_trajectory(t, end_y);
}

static float calc_speed(pos_t *pos_a, pos_t *pos_b, time_t t_time)
{
    return (float)(get_pos_distance(pos_a, pos_b) / t_time);
}

static void collect_positions(pos_t *pos_a, pos_t *pos_b, int trg, time_t *t)
{
    time_t t_start, t_end;
    int i;

    t_start = clock();
    *pos_a = get_target_pos(trg);
    i = 0;

    do
    {
        *pos_b = get_target_pos(trg);
        if (get_pos_distance(pos_a, pos_b) <= 0)
        {
            ptask_wait_for_period();
            def_missile_wait();
        }
        i++;
    } while (get_pos_distance(pos_a, pos_b) <= 0 && i < LIMIT);

    t_end = clock();
    *t = (t_end - t_start) / CLOCKS_PER_SEC;
    fprintf(stderr, "DONE\n");
}

static float get_start_x_position(int target)
{
    trajectory_t trajectory;
    time_t t_total;
    pos_t pos_a, pos_b;

    collect_positions(&pos_a, &pos_b, target, &t_total);

    trajectory.m = get_line_m(&pos_a, &pos_b);
    trajectory.angle = get_line_angle(&pos_a, &pos_b);
    trajectory.b = get_line_b_with_m(trajectory.m, &pos_a);
    trajectory.speed = calc_speed(&pos_a, &pos_b, t_total);

    return pos_a.x == pos_b.x ? get_expected_position_x(&trajectory, &pos_b) : pos_a.x;
}

static void set_missile_trajectory(missile_t *missile, float start_x)
{
    missile->partial_y = missile->y = DEF_MISSILE_START_Y;
    missile->partial_x = missile->x = (int)start_x;
    missile->angle = 90;
    missile->speed = DEF_MISSILE_SPEED;
}

ptask def_thread()
{
    missile_t *self;
    float start_x;
    int task_index;

    task_index = ptask_get_index();
    self = ptask_get_argument();

    start_x = get_start_x_position(self->target);
    set_missile_trajectory(self, start_x);

    task_missile_movement(self, task_index);

    clear_atk_missile(self);
}

static void init_def_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), DEF_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), DEF_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    params->arg = arg;
}

static int launch_def_thread(missile_t *missile)
{
    tpars params;

    init_def_params(&params, missile);

    return ptask_create_param(def_thread, &params);
}

static void init_def_missile(missile_t *missile, int index, int target)
{
    init_empty_missile(missile);
    missile->index = index;
    missile->target = target;
    missile->missile_type = DEFENDER;
}

static void launch_def_missile(int index, int target)
{
    missile_t *missile;
    int thread;

    missile = &(atk_gestor.queue[index]);
    init_def_missile(missile, index, target);

    thread = launch_def_thread(missile);
    assert(thread >= 0);
}

ptask def_launcher()
{
    int index, target;

    while (1)
    {
        target = search_screen_for_target();

        if (target != -1)
        {
            fprintf(stderr, "Target %i\n", target);
            index = request_def_launch();
            launch_def_missile(index, target);
            def_wait();
        }

        ptask_wait_for_period();
    }
}

void launch_def_launcher()
{
    int task;

    task = ptask_create_prio(def_launcher,
                             DEF_LAUNCHER_PERIOD,
                             DEF_LAUNCHER_PRIO,
                             NOW);

    assert(task >= 0);

    fprintf(stderr, "Created DEF launcher\n");
}

void delete_def_missile(int index)
{
    missile_t *missile;

    missile = &(def_gestor.queue[index]);
    missile->deleted = 1;
}