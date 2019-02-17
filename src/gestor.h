#ifndef GESTOR_H
#define GESTOR_H

#include <stdlib.h>
#include <allegro.h>
#include <semaphore.h>
#include "ptask.h"
#include "launchers.h"

// PI constant used in calculus.
#define M_PI 3.14159265358979323846
// Division factor for deltatime.
#define DELTA_FACTOR 1000

/**
 * ENVIRNOMENT PARAMETERS
 */

//  Number of priorities for environment access.
#define ENV_PRIOS 3
//  Lowest priority for environment access.
#define LOW_ENV_PRIO 2
//  Medium priority for environment access.
#define MIDDLE_ENV_PRIO 1
//  Highest priority for environment access.
#define HIGH_ENV_PRIO 0

// Value of an empty cell inside the environment.
#define EMPTY_CELL -1
// Value of a cell that is not empty but contains static data
// (wall or goal cell).
#define OTHER_CELL -2

/**
 * DISPLAY PARAMETERS
 */

// Refresh rate of the screen (updates per second)
#define REFRESH_RATE 30
// Period of the display manager task.
#define DISPLAY_PERIOD ((int)(1000 / REFRESH_RATE))
// Priority of the display manager task.
#define DISPLAY_PRIO 3

// Horizontal size of the window.
#define XWIN 640
// Vertical size of the window.
#define YWIN 480
// Thickness of the wall around the window.
#define WALL_THICKNESS 2
// Starting point of the goal for attacker missile.
#define GOAL_START_Y (YWIN * 0.8)

// Background color of the display (0 = black).
#define BKG_COLOR 0
// Color of the wall around the window (6 = brown).
#define WALL_COLOR 6
// Color of the goal for the attacker missile (10 = light green).
#define GOAL_COLOR 10
// Color of the text labels (14 = yellow).
#define LABEL_COLOR 14
// Color of the attaker missile (4 = red).
#define ATTACKER_COLOR 4
// Color of the defender missile (11 = light cyan).
#define DEFENDER_COLOR 11

// Maximum length of string of text in labels
#define LABEL_LEN 30
// Height of the rectangle used in the legend.
#define RECT_H 8
// Width of the rectangle used in the legend.
#define RECT_W ((int)(((float)RECT_H / (float)YWIN) * (float)XWIN))
// Horizontal starting point of the legend.
#define LEGEND_X (XWIN - 120)
// Vertical starting point of the legend.
#define LEGEND_Y 10
// Height of the text labels, used to calculate vertical starting point.
#define LABEL_H 10
// Horizontal starting point of the text labels.
#define LABEL_X 10
// Get vertical starting point given the number of spaces (s).
#define GET_Y_LABEL(s) (XWIN - s * LABEL_H)
// Spaces between lines in the legend.
#define SPACING 2

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