#ifndef ENV_HANDLER_H
#define ENV_HANDLER_H

#include "patriots.h"
#include "missile.h"
#include "./attacker/atk-launcher.h"
#include "./defender/def-launcher.h"
#include <allegro.h>

typedef enum {WALL, GOAL, EMPTY, ATK_MISSILE, DEF_MISSILE} cell_type;

typedef struct {
    cell_type type;
    int value;
} cell_t;

typedef struct {
    cell_t cell[XWIN][YWIN];
    sem_t mutex;
} env_t;

int check_missile_collisions(missile_t *missile);

int update_missile_position(int oldx, int oldy, missile_t *missile, int task);

void init_env(int wall_border, int goal_margin);

void draw_env(BITMAP* buffer);

#endif