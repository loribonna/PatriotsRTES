/********************************************************************
 * Lorenzo Bonicelli 2019
 * 
 * This file contains the declarations of the structures used to
 * manage the environment or the display and function prototypes
 * necessary to initialize or alter the state of such environment.
 * 
********************************************************************/

#ifndef GESTOR_H
#define GESTOR_H

#include <stdlib.h>
#include <semaphore.h>

#include "launchers.h"
#include "patriots.h"

/********************************************************************
 * ENVIRNOMENT PARAMETERS
********************************************************************/

//  Number of priorities for environment access.
#define ENV_PRIOS           3
//  Lowest priority for environment access.
#define LOW_ENV_PRIO        2
//  Medium priority for environment access.
#define MIDDLE_ENV_PRIO     1
//  Highest priority for environment access.
#define HIGH_ENV_PRIO       0

// Value of an empty cell inside the environment.
#define EMPTY_CELL          -1
// Value of a cell that is not empty but contains static data
// (wall or goal cell).
#define OTHER_CELL          -2

/********************************************************************
 * DISPLAY PARAMETERS
********************************************************************/

// Refresh rate of the screen (updates per second)
#define REFRESH_RATE        60
// Period of the display manager task.
#define DISPLAY_PERIOD      ((int)(1000 / REFRESH_RATE))
// Priority of the display manager task.
#define DISPLAY_PRIO        3
// Relative deadline of the display manager task.
#define DISPLAY_DEADLINE    (DISPLAY_PERIOD)

// Horizontal size of the window.
#define XWIN                640
// Vertical size of the window.
#define YWIN                480
// Thickness of the wall around the window.
#define WALL_THICKNESS      2
// Starting point of the goal for attacker missile.
#define GOAL_START_Y        (YWIN * 0.8)

// Background color of the display (0 = black).
#define BKG_COLOR           0
// Color of the wall around the window (6 = brown).
#define WALL_COLOR          6
// Color of the goal for the attacker missile (10 = light green).
#define GOAL_COLOR          10
// Color of the text labels (14 = yellow).
#define LABEL_COLOR         14
// Color of the attaker missile (4 = red).
#define ATTACKER_COLOR      4
// Color of the defender missile (11 = light cyan).
#define DEFENDER_COLOR      11

// Vertical starting point of the tutorial text.
#define TUTORIAL_Y          10
// Maximum length of string of text in labels.
#define LABEL_LEN           30
// Height of the rectangle used in the legend.
#define RECT_H              8
// Width of the rectangle used in the legend.
#define RECT_W              ((int)(((float)RECT_H / (float)YWIN) * (float)XWIN))
// Horizontal starting point of the legend.
#define LEGEND_X            (XWIN - 120)
// Vertical starting point of the legend.
#define LEGEND_Y            (TUTORIAL_Y + 20)
// Height of the text labels, used to calculate vertical starting point.
#define LABEL_H             10
// Horizontal starting point of the text labels.
#define LABEL_X             10
// Get vertical starting point given the number of spaces (s).
#define GET_Y_LABEL(s)      (YWIN - s * LABEL_H)
// Spaces between lines in the legend.
#define SPACING             2

// X and Y coordinate for a point in 2D.
typedef struct
{
    int x, y;
}   pos_t;

/*
 * Initialize environment and display manager.
 */
void init_gestor();

/*
 * Launch display manager task.
 */
void launch_display_manager();

/*
 * Check if a deadline was missed in the current task and print an informative
 * message.
 * 
 * message: array of characters (string) with the message to print in case
 * of a deadline miss.
 */
void check_deadline(char *message);

/*
 * Check if a deadline was missed by a messile task and print an informative
 * formatted string.
 * 
 * message: array of characters (string) with the message to print in case
 * of a deadline miss.
 * type: type of the missile base of the task.
 * index: index of the missile base of the task.
 */
void check_missile_deadline(char *message, missile_type_t type, int index);

/*
 * Update missile position in the environment and check for collisions.
 * 
 * missile: reference to the missile structure.
 * oldx: x coordinate of the past position.
 * oldy: y coordinate of the past position.
 * ~return: 1 if the given missile collides with something, else 0
 */
int update_missile_env(missile_t *missile, int oldx, int oldy);

/*
 * Search screen for a new target (attacker missile) and marks
 * it as tracked by assigning ad index <t_assign>.
 * 
 * t_assign: index to assign to the eventual found target.
 * ~return: 1 if a target was found, else 0.
 */
int search_screen_for_target(int t_assign);

/*
 * Search screen for the <target>'s current position.
 * 
 * target: index of the target to search in the screen.
 * ~return: current position of the target. If the target
 * was not found, the position returned is (-1, -1).
 */
pos_t scan_env_for_target_pos(int target);

#endif