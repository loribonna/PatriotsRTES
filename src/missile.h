#ifndef MISSILE_H
#define MISSILE_H

#include <math.h>
#include "utils.h"
#include <allegro.h>

#define MISSILE_RADIUS 2
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
    missile_type_t missile_type;
} missile_t;

int missile_inside_borders(missile_t *missile);

int draw_missile(BITMAP *buffer, missile_t *missile);

int missiles_collide(missile_t *missileA, missile_t *missileB);

void move_missile(missile_t *missile, float deltatime);

#include "env-handler.h"

#endif