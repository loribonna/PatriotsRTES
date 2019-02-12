#include "patriots.h"

void init()
{
    display_init();

    init_env();

    ptask_init(SCHED_FIFO, 1, 2);
}

void spawn_tasks()
{
    launch_display_manager();

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
        }
    } while (k != KEY_ESC);

    allegro_exit();
    return 0;
}