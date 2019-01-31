#ifndef PATRIOTS_H
#define PATRIOTS_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <allegro.h>
#include "ptask.h"
#include <assert.h>

#define XWIN 640
#define YWIN 480
#define LBOX 2
#define RBOX 638
#define UBOX 2
#define BBOX 478
#define BKG_COLOR 0

typedef struct {
    int cell[XWIN][YWIN];
    sem_t mutex;
} env_t;

#endif