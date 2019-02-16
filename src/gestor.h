#ifndef GESTOR_H
#define GESTOR_H

#include <stdlib.h>
#include <allegro.h>
#include <semaphore.h>
#include "ptask.h"
#include "launchers.h"

// Refresh rate of the screen (updates per second)
#define REFRESH_RATE 30

#define WALL_THICKNESS 2
#define GOAL_START_Y (YWIN * 0.8)
#define EMPTY_CELL -1
#define OTHER_CELL -2

#define XWIN 640
#define YWIN 480

#define LBOX 2
#define RBOX 638
#define UBOX 2
#define BBOX 478

#define BKG_COLOR 0
#define WALL_COLOR 6
#define GOAL_COLOR 10
#define LABEL_COLOR 14
#define ATTACKER_COLOR 4
#define DEFENDER_COLOR 11

#define LABEL_LEN 30
#define RECT_H 8
#define RECT_W ((int)(((float)RECT_H / (float)YWIN) * (float)XWIN))
#define LEGEND_X (XWIN - 120)
#define LEGEND_Y 10
#define LABEL_Y 10
#define LABEL_X 10
#define SPACING 2

#define get_y_label(s) (YWIN - s * LABEL_Y)

#define DISPLAY_PERIOD ((int)(1000 / REFRESH_RATE))
#define DISPLAY_PRIO 3

#define M_PI 3.14159265358979323846
#define DELTA_FACTOR 1000

#define ENV_PRIOS 3
#define LOW_ENV_PRIO 2
#define MIDDLE_ENV_PRIO 1
#define HIGH_ENV_PRIO 0

typedef struct
{
    int atk_hit;
    int def_hit;
    sem_t mutex;
} score_t;

typedef enum
{
    WALL,
    GOAL,
    EMPTY,
    ATK_MISSILE,
    DEF_MISSILE
} cell_type_t;

typedef struct
{
    cell_type_t type;
    int value, target;
} cell_t;

typedef struct
{
    int x;
    int y;
} pos_t;

typedef struct
{
    cell_t cell[XWIN][YWIN];
    int def_points, atk_points;
    int count;
    private_sem_t prio_sem[ENV_PRIOS];
    sem_t mutex;
} env_t;

void init_gestor();

float frand(float min, float max);

float get_deltatime(int task_index, int unit);

void clear_cell(int x, int y);

int check_missile_collisions(missile_t *missile);

int update_missile_env(missile_t *missile, int oldx, int oldy);

void launch_display_manager();

int check_borders(int x, int y);

int search_screen_for_target(int t_assign);

pos_t scan_env_for_target_pos(int target);

void init_empty_pos(pos_t *pos);

int is_valid_pos(pos_t *pos);

#endif