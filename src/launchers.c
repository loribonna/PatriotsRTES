/********************************************************************
 * Lorenzo Bonicelli 2019
 * 
 * This file contains the missile ang launcher management functions 
 * and tasks.
 * 
 * The missiles (attacker or defender) are stored in two separated 
 * fifo queues, managed by a fifo gestor.
 * The fifo queue gestor can operate by extracting a new free index 
 * or by refilling a precedently used one.
 * 
 * The attacker launcher waits on the queue for a new index to 
 * assign to a missile thread, that will be provided from a keyboard
 * event in the main thread.
 * The defender launcher takes an index to assign to a new untracked
 * attacker missile.
 * The refill operation is done by a missile when it collides with 
 * something and gets cleaned.
 * 
********************************************************************/

#include "launchers.h"
#include <stdlib.h>
#include "ptask.h"
#include <time.h>
#include <math.h>
#include "gestor.h"

static missile_gestor_t atk_gestor;
static missile_gestor_t def_gestor;

/********************************************************************
 * INITIALZATIONS
********************************************************************/

/*
 * Initialize a private semaphore structure.
 * 
 * private_sem_t: reference to the private semaphore to initialize.
 */
void init_private_sem(private_sem_t *p_sem)
{
    p_sem->blk = p_sem->count = 0;
    sem_init(&p_sem->sem, 0, 0);
}

/*
 * Initialize a queue gestor.
 * 
 * gestor: reference to the queue gestor to initialize.
 */
static void init_queue(fifo_queue_gestor_t *gestor)
{
    int i;

    gestor->freeIndex = 0;
    gestor->headIndex = gestor->tailIndex = -1;

    /* Initialize concatenated queue from freeIndex. */
    for (i = 0; i < N - 1; i++)
    {
        gestor->next[i] = i + 1;
    }
    gestor->next[N - 1] = -1;

    sem_init(&(gestor->mutex), 0, 1);

    init_private_sem(&gestor->read_sem);
    init_private_sem(&gestor->write_sem);
}

/*
 * Initialize a missile structure as empty.
 * 
 * missile: reference to the missile to initialize.
 */
static void init_empty_missile(missile_t *missile)
{
    sem_init(&missile->mutex, 0, 1);
    missile->deleted = 0;
    missile->speed = missile->angle = 0;
    missile->partial_x = missile->partial_y = 0;
    missile->x = missile->y = 0;
    missile->assigned_target = missile->index = -1;
}

/*
 * Initialize a missile queue gestor.
 * 
 * m_gestor: reference to the missile gestor to initialize.
 */
static void init_missiles_gestor(missile_gestor_t *m_gestor)
{
    int i;

    init_queue(&m_gestor->gestor);

    for (i = 0; i < N; i++)
    {
        init_empty_missile(&(m_gestor->queue[i]));
    }
}

/*
 * Initialize the attack launcher missile gestor structure.
 */
static void init_atk_launcher()
{
    init_missiles_gestor(&atk_gestor);
}

/*
 * Initialize the attack launcher missile gestor structure.
 */
static void init_def_launcher()
{
    init_missiles_gestor(&def_gestor);
}

/*
 * Initialize attacker and defender launchers.
 */
void init_launchers()
{
    init_atk_launcher();
    init_def_launcher();
}

/********************************************************************
 * COMMON FUNCTIONS
********************************************************************/

/*
 * Get deltatime based on current task's period.
 * 
 * task_index: index of the task from which get the period.
 * 
 */
static float get_deltatime(int task_index, int unit)
{
    return (float)ptask_get_period(task_index, unit) / DELTA_FACTOR;
}

/*
 * Get float random number between <min> and <max>.

 * min: lower bound for the generated number.
 * max: upper bound for the generated number.
 * ~return: random floating number between the extremes.
 */
static float frand(float min, float max)
{
    float   r;

    r = rand() / (float)RAND_MAX;
    return min + (max - min) * r;
}

/********************************************************************
 * QUEUE MANAGEMENT
********************************************************************/

/*
 * Add the current index to the head of the queue.
 * 
 * gestor: reference to the queue gestor.
 * index: index to push to the head.
 */
static void update_queue_head(fifo_queue_gestor_t *gestor, int index)
{
    assert(index >= 0 && gestor->read_sem.blk > 0);

    gestor->next[index] = -1;       // Added index will be the end of the queue.
    if (gestor->headIndex == -1)    // Update head if was empty,
    {
        gestor->headIndex = index;
    }
    else                            // else concatenate with previous indexes.
    {
        gestor->next[gestor->tailIndex] = index;
    }
    gestor->tailIndex = index;      // Keep track of last added index.
}

/*
 * BLOCKING: Request an attacker missile task launch.
 * Block if there are tasks to wake up.
 */
void request_atk_launch()
{
    int                 index;
    fifo_queue_gestor_t *gestor;

    gestor = &(atk_gestor.gestor);

    sem_wait(&(gestor->mutex));

    /* If there are no free indexes or blocked readers do not block. */
    if (gestor->freeIndex == -1 || !gestor->read_sem.blk)
    {
        sem_post(&(gestor->mutex));
    }
    else    
    {
        index = gestor->freeIndex;
        gestor->freeIndex = gestor->next[gestor->freeIndex];
        update_queue_head(gestor, index);   // Get and indert an index in head.
        sem_post(&(gestor->read_sem).sem);  // Mutex passed to the waken reader.
    }
}

/*
 * BLOCKING: Request a free index from the defender missile gestor.
 * Block if there are no free index available or if there are other
 * waiting tasks.
 * 
 * ~return: free index for the missile queue.
 */
int request_def_index()
{
    int                 index;
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

    assert(gestor->freeIndex >= 0);

    /* Take the first free index and point to the next one. */
    index = gestor->freeIndex;
    gestor->freeIndex = gestor->next[gestor->freeIndex];

    sem_post(&(gestor->mutex));

    return index;
}

/*
 * BLOCKING: Request a index from the head of the queue.
 * Block if there are no head index available.
 * 
 * gestor: reference to the queue gestor.
 * ~return: index for the queue.
 */
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

/*
 * Release an index from the queue specified.
 * 
 * gestor: reference to the queue gestor.
 * index: used index to release.
 */
static void splice_index(fifo_queue_gestor_t *gestor, int index)
{
    assert(index >= 0);

    /* The spliced index is set as the start of the queue. */
    gestor->next[index] = gestor->freeIndex;
    gestor->freeIndex = index;

    if (gestor->write_sem.blk)  // Mutex passing to the blocked task.
    {
        sem_post(&gestor->write_sem.sem);
    }
    else
    {
        sem_post(&(gestor->mutex));
    }
}

/********************************************************************
 * MISSILE FUNCTIONS
********************************************************************/

/*
 * Assign a target index to an attacker missile task.
 * 
 * index: index of the attacker missile.
 * target: target index to assign.
 */
void assign_target_to_atk(int index, int target)
{
    missile_t   *missile;
    missile = &(atk_gestor.queue[index]);

    missile->assigned_target = target;
}

/*
 * BLOCKING: Release the data associated with a missile structure.
 * Block if there are tasks to wake up.
 * 
 * gestor: reference to the queue gestor to release the missile index.
 * missile: reference to the missile structure to clear.
 */
static void clear_missile(missile_t *missile, fifo_queue_gestor_t *gestor)
{
    int index;

    sem_wait(&gestor->mutex);

    index = missile->index;
    init_empty_missile(missile);

    splice_index(gestor, index);
    
}

/*
 * Update missile structure position based on speed and angle.
 * 
 * missile: reference to the missile structure to update.
 * deltatime: deltatime between task activation, used to smooth
 * the movement.
 */
static void move_missile(missile_t *missile, float deltatime)
{
    float   dx, dy, angle_rad;

    angle_rad = missile->angle * (M_PI / 180);  // Convert degrees in radians.

    dx = missile->speed * cos(angle_rad);
    dy = missile->speed * sin(angle_rad);

    missile->partial_x += dx * deltatime;
    missile->partial_y += dy * deltatime;

    missile->x = (int)missile->partial_x;
    missile->y = (int)missile->partial_y;
}

/*
 * Missile task movement loop: update missile position until there
 * is a collision or the end is signaled.
 * 
 * missile: reference to the missile structure to update.
 * task_index: index of the missile task.
 */
static void task_missile_movement(missile_t *missile, int task_index)
{
    int     oldx, oldy, collided;
    float   deltatime;

    deltatime = get_deltatime(task_index, MILLI); // Task period doesn't change.

    do
    {
        oldx = missile->x;
        oldy = missile->y;

        move_missile(missile, deltatime);
        collided = update_missile_env(missile, oldx, oldy);

        ptask_wait_for_period();
    } while (!collided && !end);
}

/********************************************************************
 * ATTACK THREADS
********************************************************************/

/*
 * Initialize the attack missile task parameters.
 * 
 * params: reference to the params to initialize.
 * arg: reference to an argument to pass to the task.
 */
static void init_atk_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), ATK_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), ATK_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    params->arg = arg;
}

/*
 * Attacker missile task.
 */
static ptask atk_thread(void)
{
    missile_t   *self;
    int         task_index;

    task_index = ptask_get_index();
    self = ptask_get_argument();

    task_missile_movement(self, task_index);

    clear_missile(self, &atk_gestor.gestor);
}

/*
 * Launch a new attacker missile task given the missile structure.
 * 
 * missile: reference to the missile structure to associate with the task.
 * ~return: -1 if an error occurs with the task creation, else >0.
 */
static int launch_atk_thread(missile_t *missile)
{
    tpars   params;

    init_atk_params(&params, missile);

    return ptask_create_param(atk_thread, &params);
}

/*
 * Wait operation between attacker missile launches.
 */
static void atk_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = ATK_SLEEP_DELAY;
    nanosleep(&t, NULL);
}

/*
 * Initialize attacker missile structure with a random parameters.
 * 
 * missile: reference to the missile structure to initialize.
 */
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

/*
 * Initialize attacker missile structure.
 * 
 * missile: reference to the missile structure to initialize.
 * index: index from the missile queue of the missile.
 */
static void init_atk_missile(missile_t *missile, int index)
{
    init_empty_missile(missile);
    missile->index = index;
    missile->missile_type = ATTACKER;
    set_random_start(missile);
}

/*
 * Initialize structure and launch an attacker missile task.
 * 
 * index: index from the missile queue of the missile.
 */
static void launch_atk_missile(int index)
{
    missile_t   *missile;
    int         thread;

    missile = &(atk_gestor.queue[index]);
    init_atk_missile(missile, index);

    thread = launch_atk_thread(missile);

    assert(thread >= 0);
}

/*
 * Attack missile launcher task.
 */
static ptask atk_launcher()
{
    int index;

    while (!end)
    {
        index = get_next_index(&atk_gestor.gestor); // Wait for an index to use.
        launch_atk_missile(index);
        atk_wait();
        ptask_wait_for_period();
    }
}

/*
 * Launch attack launcher task.
 */
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

/*
 * Delete an attacker missile.
 * 
 * index: index of the missile to delete.
 */
void delete_atk_missile(int index)
{
    missile_t   *missile;

    missile = &(atk_gestor.queue[index]);
    missile->deleted = 1;
}

/********************************************************************
 * DEFENDER THREADS
********************************************************************/

/*
 * Wait operation between defender missile launches.
 */
static void def_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = DEF_SLEEP_DELAY;
    nanosleep(&t, NULL);
}

/*
 * Check if a given target index is already assigned to a 
 * defender missile task.
 * 
 * target: index to check.
 * ~return: 1 if the given index is already tracked, else 0. 
 */
int is_already_tracked(int target)
{
    int i, ret;

    if (target < 0)
    {
        return 0;
    }

    ret = 0;

    /* Check if target index is already assigned to a defender missile. */
    for (i = 0; i < N; i++)
    {
        ret |= def_gestor.queue[i].index == target;
    }

    return ret;
}

/*
 * Get euclidean distance between points.
 * 
 * pos_a: reference to a position in 2D space.
 * pos_b: reference to a position in 2D space.
 * ~return: distance between input points.
 */
static float get_pos_distance(pos_t *pos_a, pos_t *pos_b)
{
    return sqrt(pow(pos_a->x - pos_b->x, 2) + pow(pos_a->y - pos_b->y, 2));
}

/*
 * Get angular coefficient of the straight line between points.
 * 
 * pos_a: reference to a position in 2D space.
 * pos_b: reference to a position in 2D space.
 * ~return: angualr coefficient of the straight line between input points.
 */
static float get_line_m(pos_t *pos_a, pos_t *pos_b)
{
    if (pos_b->x == pos_a->x)   // If dx is 0 should not be accounted.
    {
        return 0.0;
    }
    return (float)(pos_b->y - pos_a->y) / (float)(pos_b->x - pos_a->x);
}

/*
 * Get vertical origin of the straight line between points.
 * 
 * pos_a: reference to a position in 2D space.
 * pos_b: reference to a position in 2D space.
 * ~return: vertical origin of the straight line between input points.
 */
static float get_line_b(float m, pos_t *pos)
{
    return -m * pos->x + pos->y;
}

/*
 * Get y coordinate of the trajectory from the x coordinate.
 * 
 * t: reference to the trajectory in 2D space.
 * x: x coordinate.
 * ~return: y coordinate value for the given input.
 */
static float get_y_from_trajectory(trajectory_t *t, float x)
{
    return t->m * x + t->b;
}

/*
 * Calculate expected intercept between the target trajectory given
 * its last known position. Solved by using Bisection method.
 * In-depth analysis of the algoritm in the README.
 * 
 * t: reference to the trajectory of the target missile.
 * current: reference to the last position of the target.
 * ~return: expected x coordinate of the intersection.
 */
static int get_expected_position_x(trajectory_t *t, pos_t *current)
{
    float   x, y, dsa, dsb, tmp_dsa, x_min, x_max;
    int     i;

    /* Initialized with screen limits. */
    x_min = t->m < 0 ? WALL_THICKNESS + MISSILE_RADIUS + 1 : current->x;
    x_max = t->m < 0 ? current->x : XWIN - WALL_THICKNESS - MISSILE_RADIUS - 1;
    i = 0;

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

    return (int)x;
}

/*
 * Get the speed between two points in 2D space given the time.
 * 
 * pos_a: reference to the starting position in 2D space.
 * pos_b: reference to the ending position in 2D space.
 * t_time: total time to reach the ending position from the starting.
 * ~return: averege speed between the input points.
 */
static float calc_speed(pos_t *pos_a, pos_t *pos_b, float t_time)
{
    assert(t_time > 0);
    return get_pos_distance(pos_a, pos_b) / t_time;
}

/*
 * Calculate deltatime from the start and end time.
 * 
 * t_start: starting timespec time.
 * t_end: ending timespec time.
 * ~return: deltatime in seconds from the starting to the ending time.
 */
static float calc_dt(struct timespec t_start, struct timespec t_end)
{
    return (float)(t_end.tv_sec - t_start.tv_sec) +
           (t_end.tv_nsec - t_start.tv_nsec) / 1000000000.0;
}

/*
 * Get starting and ending position of the target and mesure time. To
 * enhance the precision, keep collect samples until a given level of
 * precision or a loop limit is reached.
 * 
 * pos_a: reference to the starting position.
 * pos_b: reference to the ending position.
 * trgt: index of the target to analyze.
 * dt: reference to the deltatime between starting and ending position.
 */
static void collect_positions(pos_t *pos_a, pos_t *pos_b, int trgt, float *dt)
{
    struct timespec t_start, t_end;
    float speed_a, speed_b;
    int i;

    clock_gettime(CLOCK_MONOTONIC, &t_start);       // Use absolute time.
    *pos_a = scan_env_for_target_pos(trgt);
    speed_b = i = 0;

    do
    {
        speed_a = speed_b;
        *pos_b = scan_env_for_target_pos(trgt);

        clock_gettime(CLOCK_MONOTONIC, &t_end);
        *dt = calc_dt(t_start, t_end);
        speed_b = calc_speed(pos_a, pos_b, *dt);

        ptask_wait_for_period();                    // Let the target update.
        i++;
    } while (i < SAMPLE_LIMIT &&                    // Check upper bound,
             (fabs(speed_b - speed_a) > EPSILON ||  // precision,
              i < MIN_SAMPLES || speed_b == 0));    // lower bound.
}

/*
 * Calculate the expected x coordinate in order to intercept the target.
 * 
 * target: index of the target to analyze.
 * ~return: expected x coordinate to intercept the target.
 */
static int get_start_x_position(int target)
{
    trajectory_t    trajectory;
    float           dt;
    pos_t           pos_a, pos_b;

    collect_positions(&pos_a, &pos_b, target, &dt);

    assert(dt != 0);

    /* If the x coordinate doesn't change the calculus is useless. */
    if (pos_a.x != pos_b.x)
    {
        trajectory.m = get_line_m(&pos_a, &pos_b);
        trajectory.b = get_line_b(trajectory.m, &pos_a);
        trajectory.speed = calc_speed(&pos_a, &pos_b, dt);

        fprintf(stderr, "DEF: Calculated speed for target %i: %f\n",
                target, trajectory.speed);

        return get_expected_position_x(&trajectory, &pos_b);
    }
    return pos_a.x;
}

/*
 * Initialize defender missile structure with the given trajectory.
 * 
 * missile: reference to the missile structure to initialize.
 * start_x: horizontal starting point of the missile.
 */
static void set_missile_trajectory(missile_t *missile, int start_x)
{
    missile->partial_y = missile->y = DEF_MISSILE_START_Y;
    missile->partial_x = missile->x = start_x;
    missile->angle = 90;
    missile->speed = -DEF_MISSILE_SPEED;
}

/*
 * Defender missile task.
 */
static ptask def_thread()
{
    missile_t   *self;
    int         start_x, task_index;

    task_index = ptask_get_index();
    self = ptask_get_argument();

    /* Expected intercept calculus done before start moving. */
    start_x = get_start_x_position(self->index);
    set_missile_trajectory(self, start_x);

    task_missile_movement(self, task_index);

    clear_missile(self, &def_gestor.gestor);
}

/*
 * Initialize defender missile task parameters.
 * 
 * params: reference to the parameters to initialize.
 * arg: reference to a parameter to pass to the task.
 */
static void init_def_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), DEF_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), DEF_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    params->arg = arg;
}

/*
 * Launch a new defender missile task given the missile structure.
 * 
 * 
 * missile: reference to the missile structure to associate with the task.
 * ~return: -1 if an error occurs with the task creation.
 */
static int launch_def_thread(missile_t *missile)
{
    tpars   params;

    init_def_params(&params, missile);

    return ptask_create_param(def_thread, &params);
}

/*
 * Initialize a defender missile structure.
 */
static void init_def_missile(missile_t *missile, int index)
{
    init_empty_missile(missile);
    missile->index = index;
    missile->missile_type = DEFENDER;
}

/*
 * Initialize structure and launch an defender missile task.
 * 
 * index: index from the missile queue of the missile.
 */
static void launch_def_missile(int index)
{
    missile_t   *missile;
    int         thread;

    missile = &(def_gestor.queue[index]);
    init_def_missile(missile, index);

    thread = launch_def_thread(missile);

    assert(thread >= 0);
}

/*
 * Defender missile launcher task.
 */
static ptask def_launcher()
{
    int index;

    index = request_def_index();
    do
    {
        /* If a valid index was acquired no need to get it again. */
        index = index >= 0 ? index : request_def_index();
        if (search_screen_for_target(index))
        {
            fprintf(stderr, "DEF_LAUNCHER: Found target and assigned %i\n",
                    index);
            launch_def_missile(index);
            index = -1; // Reset index to acquire a new one.
            def_wait();
        }

        ptask_wait_for_period();

    } while (!end);
}

/*
 * Launch defender launcher task.
 */
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

/*
 * Delete an defender missile.
 * 
 * index: index of the missile to delete.
 */
void delete_def_missile(int index)
{
    missile_t   *missile;

    missile = &(def_gestor.queue[index]);
    missile->deleted = 1;
}