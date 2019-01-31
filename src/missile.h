#ifndef MISSILE_H
#define MISSILE_H

#include "patriots.h"
#include <math.h>
#include "utils.h"

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

#endif