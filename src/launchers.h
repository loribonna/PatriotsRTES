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
    float x, y;
    float angle, speed;
    int index;
    sem_t deleted;
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
    sem_t mutex_insert, mutex_remove;
    struct private_sem_t empty_sem;
    struct private_sem_t full_sem;
} fifo_queue_gestor_t;

typedef struct {
    missile_t queue[N];
    fifo_queue_gestor_t gestor;
} atk_gestor_t;

extern atk_gestor_t atk_gestor;

void init_launchers();

int is_queue_full(fifo_queue_gestor_t *gestor);

void init_queue(fifo_queue_gestor_t *gestor);

int get_next_empty_item(fifo_queue_gestor_t *gestor);

void add_full_item(fifo_queue_gestor_t *gestor, int index);

int get_next_full_item(fifo_queue_gestor_t *gestor);

void splice_full_item(fifo_queue_gestor_t *gestor, int index);

void print_missile(missile_t *missile);

void delete_missile(missile_t *missile);

void init_missile(missile_t *missile);

int is_deleted(missile_t *missile);

int draw_missile(BITMAP *buffer, int x, int y, missile_type_t type);

void move_missile(missile_t *missile, float deltatime);

int launch_atk_thread(missile_t* missile);

void init_atk_launcher();

void delete_atk_missile(int index);

void atk_missile_goal(int task);

void launch_atk_launcher();

void delete_def_missile(int index);

#include "gestor.h"

#endif