#ifndef ENV_HANDLER_H
#define ENV_HANDLER_H

#include "display.h"
#include "missile.h"
#include "./attacker/atk-launcher.h"
#include "./defender/def-launcher.h"
#include <allegro.h>

#define WALL_THICKNESS 2
#define GOAL_START_Y (YWIN * 0.8)
#define EMPTY_CELL -1
#define OTHER_CELL -2

typedef enum
{
    WALL,
    GOAL,
    EMPTY,
    ATK_MISSILE,
    DEF_MISSILE
} cell_type;

typedef struct
{
    cell_type type;
    int value;
} cell_t;

typedef struct
{
    cell_t cell[XWIN][YWIN];
    int def_points, atk_points;
    sem_t mutex;
} env_t;

extern env_t env;

int check_missile_collisions(missile_t *missile);

int update_missile_position(missile_t *missile, float deltatime);

void draw_env(BITMAP *buffer);

void init_env();

#endif