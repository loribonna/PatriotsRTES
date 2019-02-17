#ifndef LAUNCHERS_H
#define LAUNCHERS_H

#include <stdlib.h>
#include "ptask.h"

// Missile radius, used for draw a missile and check collisions.
#define MISSILE_RADIUS 5
// Max number of missile threads -> Size of the queue.
#define N 4

/**
 * ATTACK PARAMETERS
 */

// Upper extremity for random attack missile speed.
#define MAX_ATK_SPEED 100
// Lower extremity for random attack missile speed.
#define MIN_ATK_SPEED ((int)(MAX_ATK_SPEED * 0.3))
// Maximum trajectory angle for random attack missile.
#define MAX_ATK_ANGLE 30
// Delay between subsequent attack missile launches.
#define ATK_SLEEP_DELAY 500 * 1000 * 1000 // 500 milliseconds

// Period of the attack launcher task.
#define ATK_LAUNCHER_PERIOD 60
// Priority of the attack launcher task.
#define ATK_LAUNCHER_PRIO 1
// Priority of the attack missile task.
#define ATK_MISSILE_PRIO 2
// Period of the attack missile task.
#define ATK_MISSILE_PERIOD 30

/**
 * DEFENDER PARAMETERS
 */

// Common starting point for defender missiles.
#define DEF_MISSILE_START_Y (GOAL_START_Y - MISSILE_RADIUS - 1)
// Fixed defender missile speed.
#define DEF_MISSILE_SPEED 130
// Delay between subsequent defender missile launches.
#define DEF_SLEEP_DELAY 50 * 1000 * 1000 // 50 milliseconds

// Period of the defender launcher task.
#define DEF_LAUNCHER_PERIOD 40
// Priority of the defender launcher task.
#define DEF_LAUNCHER_PRIO 1
// Priority of the defender missile task.
#define DEF_MISSILE_PRIO 2
// Period of the defender missile task.
#define DEF_MISSILE_PERIOD 30

// Precision used in trajectory calculation (percentual)
#define TRAJECTORY_PRECISION 10
// Ubber extremity to limit trajectory calculation loop.
#define SAMPLE_LIMIT (2 * TRAJECTORY_PRECISION)
// Minimum number of samples to collect to calculate the trajectory
// of an attacker missile.
#define MIN_SAMPLES (SAMPLE_LIMIT / 5)
// Level of precision used in trajectory calculus.
#define EPSILON (1 / TRAJECTORY_PRECISION)

typedef enum
{
    ATTACKER,
    DEFENDER
} missile_type_t;

typedef struct
{
    sem_t sem;
    int count;
    int blk;
} private_sem_t;

typedef struct
{
    int x, y;
    float partial_x, partial_y;
    float angle, speed;
    int index;
    int deleted;
    int cleared;
    int assigned_target;
    sem_t mutex;
    missile_type_t missile_type;
} missile_t;

typedef struct
{
    int freeIndex, tailIndex, headIndex;
    int next[N];
    sem_t mutex;
    private_sem_t write_sem;
    private_sem_t read_sem;
} fifo_queue_gestor_t;

typedef struct
{
    missile_t queue[N];
    fifo_queue_gestor_t gestor;
} missile_gestor_t;

typedef struct
{
    float m, angle;
    float b;
    float speed;
} trajectory_t;

void init_launchers();

void request_atk_launch();

void delete_atk_missile(int index);

void atk_missile_goal(int task);

void launch_atk_launcher();

void launch_def_launcher();

void delete_def_missile(int index);

void init_private_sem(private_sem_t *p_sem);

void assign_target_to_atk(int index, int target);

int is_already_tracked(int target);

#include "gestor.h"

#endif