#ifndef LAUNCHERS_H
#define LAUNCHERS_H

#include <stdlib.h>
#include "ptask.h"

// Precision used in trajectory calculation (percentual)
#define TRAJECTORY_PRECISION 10

#define MAX_ATK_SPEED 100
#define MIN_ATK_SPEED ((int)(MAX_ATK_SPEED * 0.3))
#define MAX_ATK_ANGLE 30
#define ATK_SLEEP_DELAY 500 * 1000 * 1000 // 500 milliseconds

#define ATK_LAUNCHER_PERIOD 60
#define ATK_LAUNCHER_PRIO 1

#define ATK_MISSILE_PRIO 2
#define ATK_MISSILE_PERIOD 30

#define DEF_SLEEP_DELAY 50 * 1000 * 1000 // 50 milliseconds

#define DEF_LAUNCHER_PERIOD 40
#define DEF_LAUNCHER_PRIO 1

#define DEF_MISSILE_PRIO 2
#define DEF_MISSILE_PERIOD 30

#define MISSILE_RADIUS 5
#define DEF_MISSILE_START_Y (GOAL_START_Y - MISSILE_RADIUS - 1)
#define DEF_MISSILE_SPEED 130

#define WAIT_UPDATE 0
#define UPDATED 1

#define SAMPLE_LIMIT (2 * TRAJECTORY_PRECISION)
#define MIN_SAMPLES (SAMPLE_LIMIT / 5)
#define EPSILON (1 / TRAJECTORY_PRECISION)
#define N 4

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