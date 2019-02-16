#include "launchers.h"
#include <time.h>
#include <math.h>

missile_gestor_t atk_gestor;
missile_gestor_t def_gestor;

/**
 * INITIALZATIONS
 */

void init_private_sem(private_sem_t *p_sem)
{
    p_sem->blk = p_sem->count = 0;
    sem_init(&p_sem->sem, 0, 0);
}

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

    init_private_sem(&gestor->read_sem);
    init_private_sem(&gestor->write_sem);
}

static void init_empty_missile(missile_t *missile)
{
    sem_init(&missile->mutex, 0, 1);
    missile->deleted = missile->cleared = 0;
    missile->speed = missile->angle = 0;
    missile->partial_x = missile->partial_y = 0;
    missile->x = missile->y = 0;
    missile->assigned_target = missile->index = -1;
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
    assert(index >= 0 && gestor->read_sem.blk > 0);

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

    if (gestor->freeIndex == -1 || !gestor->read_sem.blk)
    {
        sem_post(&(gestor->mutex));
    }
    else
    {
        index = gestor->freeIndex;
        gestor->freeIndex = gestor->next[gestor->freeIndex];
        update_queue_head(gestor, index);
        sem_post(&(gestor->read_sem).sem);
    }
}

int request_def_launch()
{
    int index;
    fifo_queue_gestor_t *gestor;

    gestor = &(def_gestor.gestor);

    sem_wait(&(gestor->mutex));

    if (gestor->freeIndex == -1 || gestor->write_sem.blk)
    {
        gestor->write_sem.blk++;
        sem_post(&(gestor->mutex));
        sem_wait(&(gestor->write_sem.sem));
        gestor->write_sem.blk--;
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
        gestor->read_sem.blk++;
        sem_post(&(gestor->mutex));
        sem_wait(&(gestor->read_sem.sem));
        gestor->read_sem.blk--;
    }

    assert(gestor->headIndex >= 0);

    index = gestor->headIndex;
    gestor->headIndex = gestor->next[gestor->headIndex];

    sem_post(&(gestor->mutex));

    return index;
}

static void splice_index(fifo_queue_gestor_t *gestor, int index)
{
    assert(index >= 0);

    gestor->next[index] = gestor->freeIndex;
    gestor->freeIndex = index;

    if (gestor->write_sem.blk)
    {
        sem_post(&gestor->write_sem.sem);
    }
    else
    {
        sem_post(&(gestor->mutex));
    }
}

/**
 * MISSILE FUNCTIONS
 */

void assign_target_to_atk(int index, int target)
{
    missile_t *missile;
    missile = &(atk_gestor.queue[index]);

    missile->assigned_target = target;
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
    int index;

    sem_wait(&gestor->mutex);
    if (!missile->cleared)
    {
        index = missile->index;
        init_empty_missile(missile);

        splice_index(gestor, index);
    }
    else
    {
        sem_post(&gestor->mutex);
    }
}

static int move_missile(missile_t *missile, float deltatime)
{
    float dx, dy, angle_rad;

    angle_rad = missile->angle * (M_PI / 180);

    dx = missile->speed * cos(angle_rad);
    dy = missile->speed * sin(angle_rad);

    missile->partial_x += dx * deltatime;
    missile->partial_y += dy * deltatime;

    missile->x = (int)missile->partial_x;
    missile->y = (int)missile->partial_y;

    return 0;
}

static void task_missile_movement(missile_t *missile, int task_index)
{
    int oldx, oldy, collided;
    float deltatime;

    deltatime = get_deltatime(task_index, MILLI);

    do
    {
        oldx = missile->x;
        oldy = missile->y;

        move_missile(missile, deltatime);
        collided = update_missile_env(missile, oldx, oldy);

        ptask_wait_for_period();
    } while (!collided);
}
/**
 * ATTACK THREADS
 */

static pos_t get_target_pos(int target)
{
    return scan_env_for_target_pos(target);
}

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

    missile->speed = frand(MIN_ATK_SPEED, MAX_ATK_SPEED);
    missile->angle = frand(MAX_ATK_ANGLE, 180 - MAX_ATK_ANGLE);

    missile->x = (int)missile->partial_x;
    missile->y = (int)missile->partial_y;

    fprintf(stderr, "ATK: Created %i with speed %f\n",
            missile->index, missile->speed);
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

    if (target < 0)
    {
        return 0;
    }

    ret = 0;

    sem_wait(&def_gestor.gestor.mutex);

    for (i = 0; i < N; i++)
    {
        ret |= def_gestor.queue[i].index == target;
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
        return 0.0;
    }
    return (float)(pos_b->y - pos_a->y) / (float)(pos_b->x - pos_a->x);
}

static float get_line_angle(pos_t *pos_a, pos_t *pos_b)
{
    return (atan2((pos_b->y - pos_a->y), (pos_b->x - pos_a->x)) * 180) / M_PI;
}

static float get_line_b_with_m(float m, pos_t *pos)
{
    return -m * pos->x + pos->y;
}

static float get_y_from_trajectory(trajectory_t *t, float x)
{
    return t->m * x + t->b;
}

static float get_expected_position_x(trajectory_t *t, pos_t *current)
{
    float x, y, dsa, dsb, tmp_dsa, x_min, x_max;
    int i;

    i = 0;
    x_min = t->m < 0 ? WALL_THICKNESS + MISSILE_RADIUS + 1 : current->x;
    x_max = t->m < 0 ? current->x : XWIN - WALL_THICKNESS - MISSILE_RADIUS - 1;

    do
    {
        x = (x_min + x_max) / 2;
        y = get_y_from_trajectory(t, x);

        dsb = fabs(DEF_MISSILE_START_Y - y);
        dsa = sqrt(pow(y - current->y, 2) + pow(x - current->x, 2));
        tmp_dsa = (t->speed / DEF_MISSILE_SPEED) * dsb;

        x_min = t->m < 0 ? tmp_dsa < dsa ? x : x_min
                         : tmp_dsa > dsa ? x : x_min;
        x_max = t->m < 0 ? tmp_dsa > dsa ? x : x_max
                         : tmp_dsa < dsa ? x : x_max;
        i++;
    } while (fabs(tmp_dsa - dsa) > EPSILON && i < SAMPLE_LIMIT);

    return x;
}

static float calc_speed(pos_t *pos_a, pos_t *pos_b, double t_time)
{
    assert(t_time > 0);
    return (float)(get_pos_distance(pos_a, pos_b) / t_time);
}

static double calc_dt(struct timespec t_start, struct timespec t_end)
{
    return (double)(t_end.tv_sec - t_start.tv_sec) +
           (t_end.tv_nsec - t_start.tv_nsec) / 1000000000.0;
}

static void collect_positions(pos_t *pos_a, pos_t *pos_b, const int trgt, double *dt)
{
    struct timespec t_start, t_end;
    float speed_a, speed_b;
    int i;

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    *pos_a = get_target_pos(trgt);
    speed_b = i = 0;

    do
    {
        speed_a = speed_b;
        *pos_b = get_target_pos(trgt);

        clock_gettime(CLOCK_MONOTONIC, &t_end);
        *dt = calc_dt(t_start, t_end);
        speed_b = calc_speed(pos_a, pos_b, *dt);

        ptask_wait_for_period();
        i++;
    } while ((fabs(speed_b - speed_a) > EPSILON || i < MIN_SAMPLES) &&
             i < SAMPLE_LIMIT);
}

static float get_start_x_position(int target)
{
    trajectory_t trajectory;
    double dt;
    pos_t pos_a, pos_b;

    collect_positions(&pos_a, &pos_b, target, &dt);

    assert(dt != 0);

    if (pos_a.x != pos_b.x)
    {
        trajectory.m = get_line_m(&pos_a, &pos_b);
        trajectory.angle = get_line_angle(&pos_a, &pos_b);
        trajectory.b = get_line_b_with_m(trajectory.m, &pos_a);
        trajectory.speed = calc_speed(&pos_a, &pos_b, dt);

        fprintf(stderr, "DEF: Calculated speed for target %i: %f\n",
                target, trajectory.speed);

        return get_expected_position_x(&trajectory, &pos_b);
    }
    return pos_a.x;
}

static void set_missile_trajectory(missile_t *missile, float start_x)
{
    missile->partial_y = missile->y = DEF_MISSILE_START_Y;
    missile->partial_x = missile->x = (int)start_x;
    missile->angle = 90;
    missile->speed = -DEF_MISSILE_SPEED;
}

ptask def_thread()
{
    missile_t *self;
    float start_x;
    int task_index;

    task_index = ptask_get_index();
    self = ptask_get_argument();

    start_x = get_start_x_position(self->index);
    set_missile_trajectory(self, start_x);

    task_missile_movement(self, task_index);
    
    clear_def_missile(self);
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

static void init_def_missile(missile_t *missile, int index)
{
    init_empty_missile(missile);
    missile->index = index;
    missile->missile_type = DEFENDER;
}

static void launch_def_missile(int index)
{
    missile_t *missile;
    int thread;

    missile = &(def_gestor.queue[index]);
    init_def_missile(missile, index);

    thread = launch_def_thread(missile);
    assert(thread >= 0);
}

ptask def_launcher()
{
    int index;

    index = request_def_launch();
    do
    {
        index = index >= 0 ? index : request_def_launch();
        if (search_screen_for_target(index))
        {
            fprintf(stderr, "DEF_LAUNCHER: Found and assign %i\n", index);
            launch_def_missile(index);
            index = -1;
            def_wait();
        }

        ptask_wait_for_period();

    } while (1);
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