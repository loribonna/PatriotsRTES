#ifndef MISSILE_H
#define MISSILE_H

#include <math.h>
#include "utils.h"
#include <allegro.h>

#define MISSILE_RADIUS 5
#define ATTACKER_COLOR 4
#define DEFENDER_COLOR 11
#define DELTA 5

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

void print_missile(missile_t *missile);

void delete_missile(missile_t *missile);

void init_missile(missile_t *missile);

int is_deleted(missile_t *missile);

int draw_missile(BITMAP *buffer, int x, int y, missile_type_t type);

void move_missile(missile_t *missile, float deltatime);

#include "env-handler.h"
#include "display.h"

#endif