#include "atk-missile.h"

ptask *atk_thread(void *arg)
{
    missile_t *self;
    int period, index, deltatime;
    int oldx, oldy;

    index = ptask_get_index();
    deltatime = get_deltatime(index, MILLI);
    self = arg;

    while (1)
    {
        oldx = self->x;
        oldy = self->y;

        move_missile(self, deltatime);

        update_missile_position(oldx, oldy, missile, index);

        ptask_wait_for_period();
    }
}

void init_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), ATK_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), ATK_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    ptask_param_arg(*params, arg);
}

int launch_atk_thread(missile_t *missile)
{
    tpars params;

    init_params(&params, missile);

    return ptask_create_param(atk_thread, &params);
}