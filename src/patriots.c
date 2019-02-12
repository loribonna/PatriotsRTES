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

    init_atk_launcher();

    launch_atk_launcher();
}

int main(void)
{
    int c, k, index;

    init();

    spawn_tasks();

    do
    {
        k = 0;
        if (keypressed())
        {
            c = readkey();
            k = c >> 8;
            if (k == KEY_SPACE && !is_query_full(&atk_gestor.gestor))
            {
                index = get_next_empty_item(&atk_gestor.gestor);
                add_full_item(&atk_gestor.gestor, index);
            }
        }

    } while (k != KEY_ESC);

    allegro_exit();
    return 0;
}