#include "atk-launcher.h"

struct
{
    missile_t queue[N];
    fifo_queue_gestor_t gestor;
} atk_gestor;

void init_atk_missile(missile_t *missile, int index)
{
    init_missile(missile);
    missile->missile_type = ATTACKER;
    set_random_start(missile);
}

void atk_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = ATK_SLEEP_DELAY;
    nanosleep(&t, NULL);
}

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
    int index, thread;

    index = get_next_empty_item(&atk_gestor.gestor);

    missile = &(atk_gestor.queue[index]);
    init_atk_missile(missile, index);

    thread = launch_atk_thread(missile);
    assert(thread >= 0);

    add_full_item(&atk_gestor.gestor, index);
}

void *atk_launcher(void *arg)
{
    int key;

    do
    {
        key = 0;
        if (keypressed())
        {
            key = readkey() >> 8;

            if (key == KEY_SPACE)
            {
                launch_atk_missile();
                atk_wait();
            }
        }
        ptask_wait_for_period();

    } while (key != KEY_ESC);

    return 0;
}

void delete_atk_missile(int index)
{
    delete_missile(&(atk_gestor.queue[index]));
}