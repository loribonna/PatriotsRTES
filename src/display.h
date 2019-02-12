#ifndef DISPLAY_H
#define DISPLAY_H

#include <assert.h>
#include <allegro.h>
#include "ptask.h"
#include <semaphore.h>

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

typedef struct {
    int atk_hit;
    int def_hit;
    sem_t mutex;
} score_t;

void display_init();

void launch_display_manager();

int check_borders(int x, int y);

void draw_wall(int x, int y, BITMAP *buffer);

void draw_goal(int x, int y, BITMAP *buffer);

void draw_labels(BITMAP *buffer, int atk_p, int def_p);

#include "env-handler.h"

#endif