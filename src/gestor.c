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
    sem_wait(&env.mutex);

    clear_to_color(buffer, BKG_COLOR);

    sem_post(&env.mutex);
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
    clear_to_color(screen, BKG_COLOR);
    install_keyboard();

    buffer = create_bitmap(XWIN, YWIN);
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
    init_env();
    display_init();
}

/**
 * COMMON FUNCTIONS
 */

void init_empty_pos(pos_t *pos)
{
    pos->x = pos->y = -1;
}

int is_valid_pos(pos_t *pos)
{
    return pos->x >= 0 && pos->y >= 0;
}

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

void clear_cell(int x, int y)
{
    sem_wait(&env.mutex);

    init_cell_empty(&(env.cell[x][y]));

    sem_post(&env.mutex);
}

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

static cell_type handle_collision_by_cell_type(cell_t *cell)
{
    cell_type type;

    type = cell->type;

    switch (type)
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
        break;
    }

    return type;
}

static int handle_collision(missile_type_t missile_type, cell_t *cell)
{
    cell_type type;

    type = handle_collision_by_cell_type(cell);

    if (type != EMPTY)
    {
        if (
            (missile_type == ATTACKER && type == DEF_MISSILE) ||
            (missile_type == DEFENDER && type == ATK_MISSILE))
        {
            def_point();
        }
        if (missile_type == ATTACKER && type == GOAL)
        {
            atk_point();
        }

        return 1;
    }

    return 0;
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
                !is_empty_cell(&(env.cell[x][y])) &&
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

static void update_missile_cell(missile_t *missile)
{
    int x, y;
    cell_type type;

    x = missile->x;
    y = missile->y;
    type = missile->missile_type == ATTACKER ? ATK_MISSILE : DEF_MISSILE;

    env.cell[x][y].value = missile->index;
    env.cell[x][y].type = type;
}

int update_missile_position(missile_t *missile, int oldx, int oldy)
{
    int collided;

    sem_wait(&env.mutex);

    init_cell_empty(&(env.cell[oldx][oldy]));

    collided = handle_collisions_around_missile(missile, MISSILE_RADIUS);

    if (!collided)
    {
        update_missile_cell(missile);
    }

    sem_post(&env.mutex);

    return collided;
}

int update_missile_env(missile_t *missile, int oldx, int oldy)
{
    int collided;

    sem_wait(&missile->mutex);

    if (missile->deleted)
    {
        sem_post(&missile->mutex);
        return 1;
    }

    collided = update_missile_position(missile, oldx, oldy);
    sem_post(&missile->mutex);

    return collided;
}

/**
 * ENVIRONMENT DRAW
 */

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

static void draw_labels(BITMAP *buffer, int atk_p, int def_p)
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

static void draw_missile(BITMAP *buffer, pos_t pos, missile_type_t type)
{
    int color;

    if (!check_borders(pos.x, pos.y))
    {
        return;
    }

    if (type == ATTACKER)
    {
        color = ATTACKER_COLOR;
    }
    else
    {
        color = DEFENDER_COLOR;
    }

    circlefill(buffer, pos.x, pos.y, MISSILE_RADIUS, color);

    return;
}

static void draw_wall(pos_t pos, BITMAP *buffer)
{
    putpixel(buffer, pos.x, pos.y, WALL_COLOR);
}

static void draw_goal(pos_t pos, BITMAP *buffer)
{
    putpixel(buffer, pos.x, pos.y, GOAL_COLOR);
}

static void draw_cell(cell_t *cell, int x, int y, BITMAP *buffer)
{
    missile_type_t m_type;
    pos_t pos;

    pos.x = x;
    pos.y = y;

    if (is_missile_cell(cell))
    {
        m_type = cell->type == ATK_MISSILE ? ATTACKER : DEFENDER;
        draw_missile(buffer, pos, m_type);
    }
    else if (is_wall_cell(cell))
    {
        draw_wall(pos, buffer);
    }
    else if (is_goal_cell(cell))
    {
        draw_goal(pos, buffer);
    }
}

void draw_env()
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

static void draw_buffer_to_screen()
{
    sem_wait(&env.mutex);

    blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);

    sem_post(&env.mutex);
}

/**
 * DISPLAY THREAD
 */

pos_t scan_env_for_target_pos(int target)
{
    pos_t pos;

    sem_wait(&env.mutex);

    for (pos.x = 0; pos.x < XWIN; pos.x++)
    {
        for (pos.y = 0; pos.y < YWIN; pos.y++)
        {
            if (env.cell[pos.x][pos.y].value == target)
            {
                sem_post(&env.mutex);
                return pos;
            }
        }
    }

    sem_post(&env.mutex);

    pos.x = -1;
    pos.y = -1;
    return pos;
}

int check_pixel(int x, int y)
{
    if (getpixel(screen, x, y) == ATTACKER_COLOR)
    {
        return !is_already_tracked(env.cell[x][y].value);
    }

    return 0;
}

int search_screen_for_target()
{
    int x, y, target;

    sem_wait(&env.mutex);

    for (y = 0; y < YWIN; y++)
    {
        for (x = 0; x < XWIN; x++)
        {
            if (check_pixel(x, y))
            {
                target = env.cell[x][y].value;
                sem_post(&env.mutex);
                return target;
            }
        }
    }

    sem_post(&env.mutex);

    return -1;
}

static ptask display_manager(void)
{
    while (1)
    {
        reset_buffer();

        draw_env();

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