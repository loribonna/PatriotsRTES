#ifndef GESTOR_H
#define GESTOR_H

#include <allegro.h>
#include <semaphore.h>
#include "ptask.h"
#include "launchers.h"

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

#define LABEL_LEN 30
#define RECT_H 8
#define RECT_W ((int)(((float)RECT_H/(float)YWIN)*(float)XWIN))
#define LEGEND_X (XWIN - 120)
#define LEGEND_Y 10
#define LABEL_Y 10
#define LABEL_X 10
#define SPACING 2

#define get_y_label(s) (YWIN - s * LABEL_Y)

#define DISPLAY_PERIOD 10
#define DISPLAY_PRIO 5

#define M_PI 3.14159265358979323846
#define DELTA_FACTOR 1000

typedef struct {
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

float frand(float min, float max);

int get_euclidean_distance(float xa, float xb, float ya, float yb);

float get_deltatime(int task_index, int unit);

int check_missile_collisions(missile_t *missile);

int update_missile_position(missile_t *missile, float deltatime);

void draw_env(BITMAP *buffer);

void init_env();

void display_init();

void launch_display_manager();

int check_borders(int x, int y);

void draw_wall(int x, int y, BITMAP *buffer);

void draw_goal(int x, int y, BITMAP *buffer);

void draw_labels(BITMAP *buffer, int atk_p, int def_p);

#endif