#include "patriots.h"

void init()
{
    allegro_init();
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, XWIN, YWIN, 0, 0);
    clear_to_color(screen, 0);
    install_keyboard();

    textout_centre_ex(screen, font, "Press SPACE", XWIN / 2, YWIN / 2,
                      14, 0);
    ptask_init(SCHED_FIFO, 1, 2);
}

void hello_task()
{
    fprintf(stderr, "CHILD");
    char *arg = (char *)ptask_get_argument();
    fprintf(stderr, "HELLO from task: %s\n", (char *)arg);
}

int main(void)
{
    init();

    int c, k;
    tpars params = TASK_SPEC_DFL;
    params.period = tspec_from(20, MILLI);
    params.rdline = tspec_from(20, MILLI);
    params.priority = 2;
    params.measure_flag = 1;
    params.act_flag = NOW;
    params.arg = "custom argument";
    params.processor = 0;
    int task = ptask_create_param(hello_task, &params);
    printf("HELLO task %i from main\n", task);

    do
    {
        k = 0;
        if (keypressed())
        {
            c = readkey();
            printf("KEY %i\n", c);
            k = c >> 8;
        }
    } while (k != KEY_ESC);
}