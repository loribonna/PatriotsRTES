#ifndef LAUNCHERS_H
#define LAUNCHERS_H

#include "ptask.h"
#include <allegro.h>

#define MAX_SPEED 100
#define MAX_ANGLE 30
#define ATK_SLEEP_DELAY 500 * 1000 * 1000 // 500 milliseconds

#define ATK_LAUNCHER_PERIOD 40
#define ATK_LAUNCHER_PRIO 4

#define ATK_MISSILE_PRIO 4
#define ATK_MISSILE_PERIOD 20

#define MISSILE_RADIUS 5
#define ATTACKER_COLOR 4
#define DEFENDER_COLOR 11
#define DELTA 5

#define N 4

typedef enum
{
    ATTACKER,
    DEFENDER
} missile_type_t;

typedef struct
{
    int x, y;
    float partial_x, partial_y;
    float angle, speed;
    int index;
    int deleted;
    int cleared;
    sem_t mutex;
    missile_type_t missile_type;
} missile_t;

struct private_sem_t
{
    sem_t s;
    int c;
};

typedef struct
{
    int freeIndex, tailIndex, headIndex;
    int next[N];
    sem_t mutex;
    struct private_sem_t write_sem;
    struct private_sem_t read_sem;
} fifo_queue_gestor_t;

typedef struct
{
    missile_t queue[N];
    fifo_queue_gestor_t gestor;
} atk_gestor_t;

extern atk_gestor_t atk_gestor;

void init_launchers();

void request_atk_launch(fifo_queue_gestor_t *gestor);

int draw_missile(BITMAP *buffer, int x, int y, missile_type_t type);

void delete_atk_missile(int index);

void atk_missile_goal(int task);

void launch_atk_launcher();

void delete_def_missile(int index);

#include "gestor.h"

#endif