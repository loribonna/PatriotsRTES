#include "patriots.h"

void init()
{
    display_init();

    init_env();

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