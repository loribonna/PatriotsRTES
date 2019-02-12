#include "atk-missile.h"

ptask atk_thread(void)
{
    missile_t *self;
    float task_index, deltatime, collided;

    collided = 0;
    task_index = ptask_get_index();
    deltatime = get_deltatime(task_index, MILLI);
    self = ptask_get_argument();

    fprintf(stderr, "ATK missile %i: position %f %f\n", self->index, self->x, self->y);

    while (!is_deleted(self) && !collided)
    {
        collided = update_missile_position(self, deltatime);

        ptask_wait_for_period();
    }

    init_missile(self);
}

void init_params(tpars *params, void *arg)
{
    ptask_param_init(*params);
    ptask_param_period((*params), ATK_MISSILE_PERIOD, MILLI);
    ptask_param_priority((*params), ATK_MISSILE_PRIO);
    ptask_param_activation((*params), NOW);
    params->arg = arg;
}

int launch_atk_thread(missile_t *missile)
{
    tpars params;

    init_params(&params, missile);

    return ptask_create_param(atk_thread, &params);
}