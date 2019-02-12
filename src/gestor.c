#include "gestor.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

BITMAP *buffer;
score_t score;
env_t env;

/**
 * INITIALZATIONS
 */

static int goal_init_check(int x, int y)
{
    return y > GOAL_START_Y;
}

static int wall_init_check(int x, int y)
{
    return x < WALL_THICKNESS ||
           y < WALL_THICKNESS ||
           XWIN - x < WALL_THICKNESS ||
           YWIN - y < WALL_THICKNESS;
}

static void reset_buffer()
{
    clear_to_color(buffer, BKG_COLOR);
}

static void init_cell_empty(cell_t *cell)
{
    cell->type = EMPTY;
    cell->value = EMPTY_CELL;
}

static void init_wall_cell(cell_t *cell)
{
    cell->type = WALL;
    cell->value = OTHER_CELL;
}

static void init_goal_cell(cell_t *cell)
{
    cell->type = GOAL;
    cell->value = OTHER_CELL;
}

static void init_cell(int x, int y)
{
    if (wall_init_check(x, y))
    {
        init_wall_cell(&(env.cell[x][y]));
    }
    else if (goal_init_check(x, y))
    {
        init_goal_cell(&(env.cell[x][y]));
    }
    else
    {
        init_cell_empty(&(env.cell[x][y]));
    }
}

static void display_init()
{
    allegro_init();
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, XWIN, YWIN, 0, 0);
    clear_to_color(screen, 0);
    install_keyboard();

    buffer = create_bitmap(XWIN, YWIN);
    reset_buffer();
}

static void init_env()
{
    int x, y;

    env.atk_points = env.def_points = 0;

    for (x = 0; x < XWIN; x++)
    {
        for (y = 0; y < YWIN; y++)
        {
            init_cell(x, y);
        }
    }

    sem_init(&env.mutex, 0, 1);
}

void init_gestor()
{
    display_init();
    init_env();
}

/**
 * COMMON FUNCTIONS
 */

int check_borders(int x, int y)
{
    return x < XWIN &&
           y < YWIN &&
           x >= 0 &&
           y >= 0;
}

float frand(float min, float max)
{
    float r;

    r = rand() / (float)RAND_MAX;
    return min + (max - min) * r;
}

float get_deltatime(int task_index, int unit)
{
    return (float)ptask_get_period(task_index, unit) / DELTA_FACTOR;
}

/**
 * CELLS CHECK
 */

static int is_empty_cell(cell_t *cell)
{
    return (cell->type == EMPTY) ||
           (cell->value == EMPTY_CELL);
}

static int is_missile_cell(cell_t *cell)
{
    return !is_empty_cell(cell) &&
           (cell->type == ATK_MISSILE || cell->type == DEF_MISSILE);
}

static int is_wall_cell(cell_t *cell)
{
    return !is_empty_cell(cell) &&
           cell->type == WALL &&
           cell->value == OTHER_CELL;
}

static int is_goal_cell(cell_t *cell)
{
    return !is_empty_cell(cell) &&
           cell->type == GOAL &&
           cell->value == OTHER_CELL;
}

/**
 * COLLISIONS MANAGEMENT
 */

static void def_point()
{
    env.def_points++;
}

static void atk_point()
{
    env.atk_points++;
}

static int handle_collision_by_cell_type(cell_t *cell)
{
    int collided;

    collided = 1;

    switch (cell->type)
    {
    case WALL:
        break;
    case GOAL:
        break;
    case DEF_MISSILE:
        delete_def_missile(cell->value);
        init_cell_empty(cell);
        break;
    case ATK_MISSILE:
        delete_atk_missile(cell->value);
        init_cell_empty(cell);
        break;
    default:
        collided = 0;
        break;
    }

    return collided;
}

static int handle_collision(missile_type_t missile_type, cell_t *cell)
{
    int collided;

    collided = handle_collision_by_cell_type(cell);

    if (collided)
    {
        if (
            (missile_type == ATTACKER && cell->type == DEF_MISSILE) ||
            (missile_type == DEFENDER && cell->type == ATK_MISSILE))
        {
            def_point();
        }
        if (missile_type == ATTACKER && cell->type == GOAL)
        {
            atk_point();
        }
    }

    return collided;
}

static int collision_around(int xa, int ya, int xb, int yb, missile_t *missile)
{
    int x, y;
    missile_type_t missile_type;

    missile_type = missile->missile_type;

    for (x = xa; x < xb; x++)
    {
        for (y = ya; y < yb; y++)
        {
            if (env.cell[x][y].value != missile->index &&
                env.cell[x][y].type != EMPTY &&
                handle_collision(missile_type, &(env.cell[x][y])))
            {
                return 1;
            }
        }
    }

    return 0;
}

static int handle_collisions_around_missile(missile_t *missile, int span)
{
    int xa, ya, xb, yb;

    if (!check_borders(missile->x, missile->y))
    {
        return 1;
    }

    xa = missile->x - span >= 0 ? missile->x - span : 0;
    ya = missile->y - span >= 0 ? missile->y - span : 0;
    xb = missile->x + span < XWIN ? missile->x + span : XWIN;
    yb = missile->y + span < YWIN ? missile->y + span : YWIN;

    return collision_around(xa, ya, xb, yb, missile);
}

static void update_cell_value(int x, int y, int value, cell_type type)
{
    env.cell[x][y].value = value;
    env.cell[x][y].type = type;
}

int update_missile_position(missile_t *missile, float deltatime)
{
    int newx, newy, oldx, oldy, type, collisions;

    oldx = (int)missile->x;
    oldy = (int)missile->y;
    type = missile->missile_type == ATTACKER ? ATK_MISSILE : DEF_MISSILE;

    sem_wait(&env.mutex);

    move_missile(missile, deltatime);

    newx = (int)missile->x;
    newy = (int)missile->y;

    init_cell_empty(&(env.cell[oldx][oldy]));

    collisions = handle_collisions_around_missile(missile, MISSILE_RADIUS);

    if (!collisions)
    {
        update_cell_value(newx, newy, missile->index, type);
    }

    sem_post(&env.mutex);

    return collisions;
}

/**
 * ENVIRONMENT DRAW
 */

static void draw_cell(cell_t *cell, int x, int y, BITMAP *buffer)
{
    missile_type_t m_type;

    if (is_missile_cell(cell))
    {
        m_type = cell->type == ATK_MISSILE ? ATTACKER : DEFENDER;
        draw_missile(buffer, x, y, m_type);
    }
    else if (is_wall_cell(cell))
    {
        draw_wall(x, y, buffer);
    }
    else if (is_goal_cell(cell))
    {
        draw_goal(x, y, buffer);
    }
}

void draw_env(BITMAP *buffer)
{
    int x, y;

    sem_wait(&env.mutex);

    for (x = 0; x < XWIN; x++)
    {
        for (y = 0; y < YWIN; y++)
        {
            if (!is_empty_cell(&(env.cell[x][y])))
            {
                draw_cell(&(env.cell[x][y]), x, y, buffer);
            }
        }
    }

    draw_labels(buffer, env.atk_points, env.def_points);

    sem_post(&env.mutex);
}

/**
 * SCREEN DRAW
 */

static void draw_buffer_to_screen()
{
    sem_wait(&env.mutex);

    blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);

    sem_post(&env.mutex);
}

void draw_wall(int x, int y, BITMAP *buffer)
{
    putpixel(buffer, x, y, WALL_COLOR);
}

void draw_goal(int x, int y, BITMAP *buffer)
{
    putpixel(buffer, x, y, GOAL_COLOR);
}

static void draw_legend(BITMAP *buffer, int spaces, int color, char *label)
{
    int divergence, y_start, y_end;

    divergence = spaces * (SPACING + RECT_H);
    y_end = LEGEND_Y + divergence + RECT_H;
    y_start = LEGEND_Y + divergence;

    rectfill(buffer, LEGEND_X - RECT_W, y_end, LEGEND_X, y_start, color);
    textout_ex(buffer, font, label,
               LEGEND_X + SPACING, y_start, LABEL_COLOR, BKG_COLOR);
}

static void draw_legends(BITMAP *buffer)
{
    draw_legend(buffer, 0, GOAL_COLOR, ": GOAL");

    draw_legend(buffer, 1, WALL_COLOR, ": WALL");

    draw_legend(buffer, 2, ATTACKER_COLOR, ": ATK MISSILE");

    draw_legend(buffer, 3, DEFENDER_COLOR, ": DEF MISSILE");
}

void draw_labels(BITMAP *buffer, int atk_p, int def_p)
{
    char s[LABEL_LEN];

    textout_centre_ex(buffer, font, "Press SPACE", XWIN / 2, 20,
                      LABEL_COLOR, BKG_COLOR);

    sprintf(s, "Attack points: %i", atk_p);
    textout_ex(buffer, font, s, LABEL_X,
               get_y_label(1), LABEL_COLOR, BKG_COLOR);

    sprintf(s, "Defender points: %i", def_p);
    textout_ex(buffer, font, s, LABEL_X,
               get_y_label(2), LABEL_COLOR, BKG_COLOR);

    draw_legends(buffer);
}

/**
 * THREAD MANAGERS
 */

static ptask display_manager(void)
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