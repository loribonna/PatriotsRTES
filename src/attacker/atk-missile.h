#ifndef ATK_MISSILE_H
#define ATK_MISSILE_H

#include "../utils.h"
#include "../missile.h"
#include "ptask.h"
#include "atk-launcher.h"

#define ATK_MISSILE_PRIO 4
#define ATK_MISSILE_PERIOD 20

int launch_atk_thread(missile_t* missile);

#endif