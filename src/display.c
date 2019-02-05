#include "display.h"

BITMAP *buffer;
score_t score;

void display_init()
{
    allegro_init();
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, XWIN, YWIN, 0, 0);
    clear_to_color(screen, 0);
    install_keyboard();

    buffer = create_bitmap(XWIN, YWIN);
    reset_buffer();

    textout_centre_ex(screen, font, "Press SPACE", XWIN / 2, YWIN / 2,
                      14, 0);
}

void draw_buffer_to_screen()
{
    blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
}

void reset_buffer()
{
    clear_to_color(buffer, BKG_COLOR);
}

ptask display_manager(void)
{
    while (1)
    {
        reset_buffer();

        draw_env(buffer);

        draw_buffer_to_screen();

        ptask_wait_for_period();
    }
}

int launch_display_manager()
{
    tpars params;

    init_params(&params);

    return ptask_create_prio(display_manager,
                             DISPLAY_PERIOD,
                             DISPLAY_PRIO,
                             NOW);
}