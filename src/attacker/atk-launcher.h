#ifndef ATK_LAUNCHER
#define ATK_LAUNCHER

#include "../missile.h"
#include <allegro.h>
#include "../fifo-queue.h"
#include "atk-missile.h"

#define MAX_SPEED 1
#define MAX_ANGLE 30

void atk_missile_hit(int task);

#endif