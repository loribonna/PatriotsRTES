#ifndef ATK_LAUNCHER
#define ATK_LAUNCHER

#include "../missile.h"
#include <allegro.h>
#include "../fifo-queue.h"
#include "atk-missile.h"
#include <time.h>

#define MAX_SPEED 1
#define MAX_ANGLE 30
#define ATK_SLEEP_DELAY 500 * 1000 * 1000 // 500 milliseconds

#define ATK_LAUNCHER_PERIOD 40
#define ATK_LAUNCHER_PRIO 4

void delete_atk_missile(int index);

void atk_missile_goal(int task);

void launch_atk_launcher();

#endif