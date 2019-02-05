#include "display.h"

BITMAP *buffer;
score_t score;

void reset_buffer()
{
    clear_to_color(buffer, BKG_COLOR);
}

void display_init()
{
    allegro_init();
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, XWIN, YWIN, 0, 0);
    clear_to_color(screen, 0);
    install_keyboard();

    buffer = create_bitmap(XWIN, YWIN);
    reset_buffer();
    
    textout_centre_ex(screen, font, "Press SPACE", XWIN / 2, 20,
                      TEXT_COLOR, BKG_COLOR);
}

int check_borders(int x, int y)
{
    return x >= XWIN ||
           y >= YWIN ||
           x < 0 ||
           y < 0;
}

void draw_buffer_to_screen()
{
    blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
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
    return ptask_create_prio(display_manager,
                             DISPLAY_PERIOD,
                             DISPLAY_PRIO,
                             NOW);
}

void draw_wall(int x, int y, BITMAP *buffer)
{
    putpixel(buffer, x, y, WALL_COLOR);
}

void draw_goal(int x, int y, BITMAP *buffer)
{
    putpixel(buffer, x, y, GOAL_COLOR);
}