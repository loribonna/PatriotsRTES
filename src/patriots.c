
#include "patriots.h"
#include <stdio.h>
#include <allegro.h>
#include "ptask.h"
#include "launchers.h"
#include "gestor.h"

// Flag used to end all tasks loops.
int end;

/*
 * Initialize all system.
 */
void init()
{
    end = 0;

    init_gestor();

    init_launchers();

    ptask_init(SCHED_RR, GLOBAL, NO_PROTOCOL);

    fprintf(stderr, "Launching PATRIOTS with time scaling: %f\n", TIME_SCALE);
}

/*
 * Spawn main tasks: display, defender and attacker launchers.
 */
void spawn_tasks()
{
    launch_display_manager();

    launch_def_launcher();
    launch_atk_launcher();
}

/*
 * Main function, responsible to initializing the system, spawning
 * the main tasks and check for keyboard events.
 */
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

    end = 1;
    allegro_exit();
    return 0;
}