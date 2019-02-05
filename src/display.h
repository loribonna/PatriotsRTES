#ifndef DISPLAY_H
#define DISPLAY_H

#include <allegro.h>
#include "ptask.h"
#include <semaphore.h>
#include "env-handler.h"

#define XWIN 640
#define YWIN 480

#define LBOX 2
#define RBOX 638
#define UBOX 2
#define BBOX 478

#define BKG_COLOR 0

#define DISPLAY_PERIOD 10
#define DISPLAY_PRIO 5

typedef struct {
    int atk_hit;
    int def_hit;
    sem_t mutex;
} score_t;

void display_init();

int launch_display_manager();

#endif