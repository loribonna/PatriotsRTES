/********************************************************************
 * Lorenzo Bonicelli 2019
 * 
 * This file contains the environment manager functions and
 * the display manager functions and task.
 * 
 * The only shared structure is "env", which can be accessed by
 * calling the function "access_env" specifying a priority. 
 * After an access, the shared structure is released by calling
 * the function "release_env" with the same priority used to access.
 * 
********************************************************************/

#include "gestor.h"
#include <stdio.h>
#include "ptask.h"
#include <allegro.h>
#include <assert.h>
#include <math.h>

// Global environment used to maintain the status of the system
static env_t   env;

/*
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

static void reset_buffer(BITMAP *buffer)
{
    clear_to_color(buffer, BKG_COLOR);
}

static void init_cell_empty(cell_t *cell)
{
    cell->type = EMPTY;
    cell->target = cell->value = EMPTY_CELL;
}

static void init_wall_cell(cell_t *cell)
{
    cell->type = WALL;
    cell->target = cell->value = OTHER_CELL;
}

static void init_goal_cell(cell_t *cell)
{
    cell->type = GOAL;
    cell->target = cell->value = OTHER_CELL;
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
}

static void init_env()
{
    int x, y, i;

    env.atk_points = env.def_points = 0;

    for (x = 0; x < XWIN; x++)
    {
        for (y = 0; y < YWIN; y++)
        {
            init_cell(x, y);
        }
    }

    for (i = 0; i < ENV_PRIOS; i++)
    {
        init_private_sem(&(env.prio_sem[i]));
    }

    env.count = 0;

    sem_init(&env.mutex, 0, 1);
}

/*
 * Initialize environment and display manager.
 */
void init_gestor()
{
    init_env();
    display_init();
}

/*
 * COMMON FUNCTIONS
*/

static cell_type_t missile_to_cell_type(missile_type_t m_type)
{
    return m_type == ATTACKER ? ATK_MISSILE : DEF_MISSILE;
}

/*
 * Check if the given position falls inside window borders.
 * 
 * x: x coordinate of the position to check.
 * y: y coordinate of the position to check.
 * ~return: 1 if the given position falls inside window
 * borders, else 0.
 */
int check_borders(int x, int y)
{
    return x < XWIN &&
           y < YWIN &&
           x >= 0 &&
           y >= 0;
}

/*
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

/*
 * ENV ACCESS
*/

static void access_env(int prio)
{
    int lock, p;

    sem_wait(&env.mutex);
    lock = 0;

    for (p = prio; p >= 0; p--)
    {
        lock |= env.prio_sem[p].blk;
    }

    if (env.count || lock)
    {
        env.prio_sem[prio].blk++;
        sem_post(&env.mutex);
        sem_wait(&(env.prio_sem[prio].sem));
        env.prio_sem[prio].blk--;
    }
    env.count++;

    sem_post(&env.mutex);
}

static void release_env(int prio)
{
    int next_prio;

    sem_wait(&env.mutex);

    env.count--;

    next_prio = prio;
    do
    {
        next_prio = (next_prio + 1) % ENV_PRIOS;
        if (env.prio_sem[next_prio].blk)
        {
            sem_post(&(env.prio_sem[next_prio].sem));
            return;
        }
    } while (next_prio != prio);

    sem_post(&env.mutex);
}

/*
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

static cell_type_t handle_collision_by_cell_type(cell_t *cell)
{
    cell_type_t type;

    type = cell->type;

    switch (type)
    {
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
    cell_type_t type;

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
    int             x, y;
    missile_type_t  missile_type;
    cell_type_t     cell_type;

    missile_type = missile->missile_type;
    cell_type = missile_to_cell_type(missile_type);

    for (x = xa; x < xb; x++)
    {
        for (y = ya; y < yb; y++)
        {
            if ((env.cell[x][y].value != missile->index ||
                 env.cell[x][y].type != cell_type) &&
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
    int         x, y;
    cell_type_t type;

    x = missile->x;
    y = missile->y;
    type = missile_to_cell_type(missile->missile_type);

    env.cell[x][y].value = missile->index;
    env.cell[x][y].type = type;
    env.cell[x][y].target = missile->assigned_target;
}

static int update_missile_position(missile_t *missile, int oldx, int oldy)
{
    int collided;

    init_cell_empty(&(env.cell[oldx][oldy]));

    collided = handle_collisions_around_missile(missile, MISSILE_RADIUS);

    if (!collided)
    {
        update_missile_cell(missile);
    }

    return collided;
}

/*
 * Update missile position in the environment and check for collisions.
 * 
 * missile: reference to the missile structure.
 * oldx: x coordinate of the past position.
 * oldy: y coordinate of the past position.
 * ~return: 1 if the given missile collides with something, else 0
 */
int update_missile_env(missile_t *missile, int oldx, int oldy)
{
    int collided;

    access_env(MIDDLE_ENV_PRIO);

    if (missile->deleted)
    {
        collided = 1;
    }
    else
    {
        collided = update_missile_position(missile, oldx, oldy);
    }

    release_env(MIDDLE_ENV_PRIO);

    return collided;
}

/*
 * Search screen for the <target>'s current position.
 * 
 * target: index of the target to search in the screen.
 * ~return: current position of the target. If the target
 * was not found, the position returned is (-1, -1).
 */
pos_t scan_env_for_target_pos(int target)
{
    pos_t   pos;

    access_env(LOW_ENV_PRIO);

    for (pos.x = 0; pos.x < XWIN; pos.x++)
    {
        for (pos.y = 0; pos.y < YWIN; pos.y++)
        {
            if (env.cell[pos.x][pos.y].target == target)
            {
                release_env(LOW_ENV_PRIO);
                return pos;
            }
        }
    }

    release_env(LOW_ENV_PRIO);

    pos.x = -1;
    pos.y = -1;
    return pos;
}

static int check_pixel(int x, int y)
{
    cell_t  cell = env.cell[x][y];

    if (getpixel(screen, x, y) == ATTACKER_COLOR &&
        cell.value >= 0)
    {
        return !is_already_tracked(cell.target);
    }

    return 0;
}

/*
 * Search screen for a new target (attacker missile) and marks
 * it as tracked by assigning ad index <t_assign>.
 * 
 * t_assign: index to assign to the eventual found target.
 * ~return: 1 if a target was found, else 0.
 */
int search_screen_for_target(int t_assign)
{
    int x, y;

    access_env(LOW_ENV_PRIO);

    for (y = 0; y < YWIN; y++)
    {
        for (x = 0; x < XWIN; x++)
        {
            if (check_pixel(x, y))
            {
                assign_target_to_atk(env.cell[x][y].value, t_assign);
                env.cell[x][y].target = t_assign;
                release_env(LOW_ENV_PRIO);
                return 1;
            }
        }
    }

    release_env(LOW_ENV_PRIO);

    return 0;
}

/*
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
    char    s[LABEL_LEN];

    textout_centre_ex(buffer, font, "Press SPACE", XWIN / 2, 20,
                      LABEL_COLOR, BKG_COLOR);

    sprintf(s, "Attack points: %i", atk_p);
    textout_ex(buffer, font, s, LABEL_X,
               GET_Y_LABEL(1), LABEL_COLOR, BKG_COLOR);

    sprintf(s, "Defender points: %i", def_p);
    textout_ex(buffer, font, s, LABEL_X,
               GET_Y_LABEL(2), LABEL_COLOR, BKG_COLOR);

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
    missile_type_t  m_type;
    pos_t           pos;

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

static void draw_env(BITMAP *buffer)
{
    int x, y;

    access_env(HIGH_ENV_PRIO);

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

    release_env(HIGH_ENV_PRIO);
}

static void draw_buffer_to_screen(BITMAP *buffer)
{
    blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
}

/*
 * DISPLAY THREAD
*/

static ptask display_manager(void)
{
    BITMAP  *buffer;
    buffer = create_bitmap(XWIN, YWIN);

    while (!end)
    {
        reset_buffer(buffer);

        draw_env(buffer);

        draw_buffer_to_screen(buffer);

        ptask_wait_for_period();
    }
}

/*
 * Launch display manager task.
 */
void launch_display_manager()
{
    int task;

    task = ptask_create_prio(display_manager,
                             DISPLAY_PERIOD,
                             DISPLAY_PRIO,
                             NOW);

    assert(task >= 0);

    fprintf(stderr, "Created DISPLAY manager with period: %i\n",
            DISPLAY_PERIOD);
}