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
                      LABEL_COLOR, BKG_COLOR);
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

void launch_display_manager()
{
    int task;

    task = ptask_create_prio(display_manager,
                             DISPLAY_PERIOD,
                             DISPLAY_PRIO,
                             NOW);

    assert(task >= 0);

    fprintf(stderr, "Created DISPLAY manager\n");
}

void draw_wall(int x, int y, BITMAP *buffer)
{
    putpixel(buffer, x, y, WALL_COLOR);
}

void draw_goal(int x, int y, BITMAP *buffer)
{
    putpixel(buffer, x, y, GOAL_COLOR);
}

void draw_legend_label(BITMAP *buffer, int spaces, int color, char *label)
{
    int divergence, y_start, y_end;

    divergence = spaces * (SPACING + RECT_H);
    y_end = LEGEND_Y + divergence + RECT_H;
    y_start = LEGEND_Y + divergence;

    rectfill(buffer, LEGEND_X - RECT_W, y_end, LEGEND_X, y_start, color);
    textout_ex(buffer, font, label,
               LEGEND_X + SPACING, y_start, LABEL_COLOR, BKG_COLOR);
}

void draw_legends(BITMAP *buffer)
{

    draw_legend_label(buffer, 0, GOAL_COLOR, ": GOAL");

    draw_legend_label(buffer, 1, WALL_COLOR, ": WALL");

    draw_legend_label(buffer, 2, ATTACKER_COLOR, ": ATK MISSILE");

    draw_legend_label(buffer, 3, DEFENDER_COLOR, ": DEF MISSILE");
}

void draw_labels(BITMAP *buffer, int atk_p, int def_p)
{
    char s[LABEL_LEN];

    sprintf(s, "Attack points: %i", atk_p);
    textout_ex(buffer, font, s, LABEL_X,
               get_y_label(1), LABEL_COLOR, BKG_COLOR);

    sprintf(s, "Defender points: %i", def_p);
    textout_ex(buffer, font, s, LABEL_X,
               get_y_label(2), LABEL_COLOR, BKG_COLOR);

    draw_legends(buffer);
}