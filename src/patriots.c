#include "patriots.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void init()
{
    init_gestor();

    init_launchers();

    ptask_init(SCHED_FIFO, GLOBAL, NO_PROTOCOL);
}

void spawn_tasks()
{
    launch_display_manager();

    launch_def_launcher();
    launch_atk_launcher();
}

int main(void)
{
    int c, k;

    init();

    spawn_tasks();

    do
    {
        k = 0;
        if (keypressed())
        {
            c = readkey();
            k = c >> 8;
            if (k == KEY_SPACE)
            {
                request_atk_launch();
            }
        }

    } while (k != KEY_ESC);

    allegro_exit();
    return 0;
}