#include "atk-launcher.h"

struct
{
    missile_t queue[N];
    fifo_queue_gestor_t gestor;
} atk_gestor;

void set_random_start(missile_t *missile)
{
    missile->x = rand() % XWIN;
    missile->y = 0;

    missile->speed = rand() % MAX_SPEED;
    missile->angle = frand(-MAX_ANGLE, MAX_ANGLE);
}

// BLOCKING if the queue is full
void launch_atk_missile()
{
    missile_t *missile;
    int index;

    index = getNextEmptyItem(&atk_gestor.gestor);

    missile = &(atk_gestor.queue[index]);

    missile->missile_type = ATTACKER;
    set_random_start(missile);

    if (launch_atk_thread(missile))
    {
        addFullItem(&atk_gestor.gestor, index);
    }
}

void *atk_launcher(void *arg)
{
    int key, index;

    do
    {
        key = 0;
        if (keypressed())
        {
            key = readkey() >> 8;

            if (key == KEY_SPACE)
            {
                launch_atk_missile();
            }
        }
    } while (key != KEY_ESC);
}