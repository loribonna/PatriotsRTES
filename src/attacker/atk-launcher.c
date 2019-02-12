#include "atk-launcher.h"

atk_gestor_t atk_gestor;

void init_atk_launcher()
{
    init_queue(&atk_gestor.gestor);
}

void atk_wait()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = ATK_SLEEP_DELAY;
    nanosleep(&t, NULL);
    fprintf(stderr, "WAKEN\n");
}

void set_random_start(missile_t *missile)
{
    missile->x = rand() % XWIN;
    missile->y = WALL_THICKNESS + MISSILE_RADIUS + 1;

    missile->speed = frand(1, MAX_SPEED);
    missile->angle = frand(MAX_ANGLE, 180 - MAX_ANGLE);
}

void init_atk_missile(missile_t *missile, int index)
{
    init_missile(missile);
    missile->index = index;
    missile->missile_type = ATTACKER;
    set_random_start(missile);
}

void launch_atk_missile(int index)
{
    missile_t *missile;
    int thread;

    missile = &(atk_gestor.queue[index]);
    init_atk_missile(missile, index);

    thread = launch_atk_thread(missile);
    assert(thread >= 0);
}

ptask atk_launcher()
{
    int index;

    while (1)
    {
        index = get_next_full_item(&atk_gestor.gestor);
        fprintf(stderr, "Launching ATK missile index: %i\n", index);
        launch_atk_missile(index);
        atk_wait();
    }
}

void launch_atk_launcher()
{
    int task;

    task = ptask_create_prio(atk_launcher,
                             ATK_LAUNCHER_PERIOD,
                             ATK_LAUNCHER_PRIO,
                             NOW);

    assert(task >= 0);

    fprintf(stderr, "Created ATK launcher\n");
}

void delete_atk_missile(int index)
{
    delete_missile(&(atk_gestor.queue[index]));
    splice_full_item(&atk_gestor.gestor, index);
}