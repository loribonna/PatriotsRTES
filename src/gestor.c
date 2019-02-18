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

// Global environment used to maintain the status of the system.
static env_t   env;

/********************************************************************
 * INITIALZATIONS
********************************************************************/

/*
 * Check if current point belongs to the goal.
 * 
 * y: y coordinate of the point.
 * ~return: 1 if the y value matches, else 0.
 */
static int goal_init_check(int y)
{
    return y > GOAL_START_Y;
}

/*
 * Check if current point belongs to the wall.
 * 
 * x: x coordinate of the point.
 * y: y coordinate of the point.
 * ~return: 1 if the y value matches, else 0.
 */
static int wall_init_check(int x, int y)
{
    return x < WALL_THICKNESS ||
           y < WALL_THICKNESS ||
           XWIN - x < WALL_THICKNESS ||
           YWIN - y < WALL_THICKNESS;
}

/*
 * Initialize an empty cell.
 * 
 * cell: reference to the cell.
 */
static void init_cell_empty(cell_t *cell)
{
    cell->type = EMPTY;
    cell->target = cell->value = EMPTY_CELL;
}

/*
 * Initialize a wall cell.
 * 
 * cell: reference to the cell.
 */
static void init_wall_cell(cell_t *cell)
{
    cell->type = WALL;
    cell->target = cell->value = OTHER_CELL;
}

/*
 * Initialize a goal cell.
 * 
 * cell: reference to the cell.
 */
static void init_goal_cell(cell_t *cell)
{
    cell->type = GOAL;
    cell->target = cell->value = OTHER_CELL;
}

/*
 * Initialize a cell based on its position.
 * 
 * x: x coordinate of the cell.
 * y: y coordinate of the cell.
 */
static void init_cell(int x, int y)
{
    if (wall_init_check(x, y))
    {
        init_wall_cell(&(env.cell[x][y]));
    }
    else if (goal_init_check(y))
    {
        init_goal_cell(&(env.cell[x][y]));
    }
    else
    {
        init_cell_empty(&(env.cell[x][y]));
    }
}

/*
 * Initialize display. The 'set_gfx_mode' can cause a crash if the 
 * graphic mode is not supported.
 */
static void display_init()
{
    allegro_init();
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, XWIN, YWIN, 0, 0);
    clear_to_color(screen, BKG_COLOR);
    install_keyboard();
}

/*
 * Initialize environment: cells, scores and semaphores.
 */
static void init_env()
{
    int x, y, i;

    env.atk_points = env.def_points = 0;

    /* Initialize every cell of the environment. */
    for (x = 0; x < XWIN; x++)
    {
        for (y = 0; y < YWIN; y++)
        {
            init_cell(x, y);
        }
    }

    /* Initialize the private semaphore to access the environment. */
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

/********************************************************************
 * COMMON FUNCTIONS
********************************************************************/

/*
 * Reset the buffer to the background color.
 * 
 * buffer: reference to the buffer.
 */
static void reset_buffer(BITMAP *buffer)
{
    clear_to_color(buffer, BKG_COLOR);
}

/*
 * Convert missile type in the corresponding cell type.
 * 
 * m_type: type of missile.
 * ~return: type of cell relative to the missile type.
 */
static cell_type_t missile_to_cell_type(missile_type_t m_type)
{
    return m_type == ATTACKER ? ATK_MISSILE : DEF_MISSILE;
}

/*
 * Convert cell type in the corresponding missile type. 
 * 
 * cell_type: type of the cell.
 * ~return: type of missile relative to the cell type.
 */
static missile_type_t cell_to_missile_type(cell_type_t cell_type) {
    return cell_type == ATK_MISSILE ? ATTACKER : DEFENDER;
}

/*
 * Check if the given position falls inside window borders.
 * 
 * x: x coordinate of the position to check.
 * y: y coordinate of the position to check.
 * ~return: 1 if the given position falls inside window borders, else 0.
 */
int check_borders(int x, int y)
{
    return x < XWIN &&
           y < YWIN &&
           x >= 0 &&
           y >= 0;
}

/********************************************************************
 * CELLS CHECKS
********************************************************************/

/*
 * Check if the cell is empty.
 * 
 * cell: reference to the cell.
 * ~return: 1 if the cell is empty, else 0.
 */
static int is_empty_cell(cell_t *cell)
{
    return (cell->type == EMPTY) ||
           (cell->value == EMPTY_CELL);
}

/*
 * Check if the cell containes missile data.
 * 
 * cell: reference to the cell.
 * ~return: 1 if the cell is a missile, else 0.
 */
static int is_missile_cell(cell_t *cell)
{
    return !is_empty_cell(cell) &&
           (cell->type == ATK_MISSILE || cell->type == DEF_MISSILE);
}

/*
 * Check if the cell is a wall cell.
 * 
 * cell: reference to the cell.
 * ~return: 1 if the cell is a wall, else 0.
 */
static int is_wall_cell(cell_t *cell)
{
    return !is_empty_cell(cell) &&
           cell->type == WALL &&
           cell->value == OTHER_CELL;
}

/*
 * Check if the cell is a goal cell.
 * 
 * cell: reference to the cell.
 * ~return: 1 if the cell is a goal, else 0.
 */
static int is_goal_cell(cell_t *cell)
{
    return !is_empty_cell(cell) &&
           cell->type == GOAL &&
           cell->value == OTHER_CELL;
}

/********************************************************************
 * ENVIRONMENT ACCESS
********************************************************************/

/*
 * BLOCKING: Controls access to the environment structure.
 * 
 * prio: priority to request the access.
 */
static void access_env(int prio)
{
    int lock, p;

    sem_wait(&env.mutex);
    lock = 0;

    /* Check for blocked lower prio tasks. */
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

/*
 * BLOCKING: Release environment shared structure.
 * 
 * prio: priority of the precedent access.
 */
static void release_env(int prio)
{
    int next_prio;

    sem_wait(&env.mutex);

    env.count--;

    /* Wake a blocked task starting by the next one with lower prio. */
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

/********************************************************************
 * COLLISIONS MANAGEMENT
********************************************************************/

/*
 * Signal a defender point.
 */
static void def_point()
{
    env.def_points++;
}

/*
 * Signal an attacker point.
 */
static void atk_point()
{
    env.atk_points++;
}

/*
 * Handle collision with a single cell.
 * 
 * ~return: type of the given cell.
 */
static cell_type_t handle_collision_by_cell_type(cell_t *cell)
{
    cell_type_t type;

    type = cell->type;

    switch (type)
    {
    case DEF_MISSILE:   // Collision with defender missile.
        delete_def_missile(cell->value);
        init_cell_empty(cell);
        break;
    case ATK_MISSILE:   // Collision with attacker missile.
        delete_atk_missile(cell->value);
        init_cell_empty(cell);
        break;
    default:
        break;
    }

    return type;
}

/*
 * Check missile collision with a cell and update score.
 * 
 * missile_type: type of the missile colliding.
 * cell: reference to the colliding cell.
 * ~return: 1 if there was a collision, else 0.
 */
static int handle_collision(missile_type_t missile_type, cell_t *cell)
{
    cell_type_t type;

    type = handle_collision_by_cell_type(cell);

    if (type != EMPTY)
    {
        /* Defender point if a defender and an attacker missile collide. */
        if ((missile_type == ATTACKER && type == DEF_MISSILE) ||
            (missile_type == DEFENDER && type == ATK_MISSILE))
        {
            def_point();
        }
        /* Attacker point if an attacker missile collides with goal. */
        if (missile_type == ATTACKER && type == GOAL)
        {
            atk_point();
        }

        return 1;
    }

    return 0;
}

/*
 * Check and handle collisions around a missile relative to its position.
 * 
 * missile: reference the missile.
 * xa: horizontal starting point around the missile position.
 * xb: horizontal ending point around the missile position.
 * ya: vertical starting point around the missile position.
 * yb: vertical ending point around the missile position.
 * ~return: 1 if there was a collision, else 0.
 */
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
            /* Check cell value and cell type to avoid self collisions. */
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

/*
 * Handle collisions around a missile.
 * 
 * missile: reference the missile.
 * span: number of cells around the missile to check.
 * ~return: 1 if there was a collision, else 0.
 */
static int handle_collisions_around_missile(missile_t *missile, int span)
{
    int xa, ya, xb, yb;

    /* Prevent the missile to move outside borders. */
    if (!check_borders(missile->x, missile->y))
    {
        return 1;
    }

    /* Get start and ending point around missile. */
    xa = missile->x - span >= 0 ? missile->x - span : 0;
    ya = missile->y - span >= 0 ? missile->y - span : 0;
    xb = missile->x + span < XWIN ? missile->x + span : XWIN;
    yb = missile->y + span < YWIN ? missile->y + span : YWIN;

    return collision_around(xa, ya, xb, yb, missile);
}

/*
 * Set cell in the environment corresponding to the missile position.
 * 
 * missile: reference the missile.
 */
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

/*
 * Update a missile position in the environment.
 * 
 * missile: reference the missile.
 * oldx: last horizontal coordinate of the missile.
 * oldy: last vertical coordinate of the missile.
 * ~return: 1 if there was a collision, else 0.
 */
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

    /* Avoid to update missile position if was deleted. */
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

/*
 * Check if the cell contains an untracked attacker missile.
 * 
 * x: horizontal coordinate of the cell.
 * y: vertical coordinate of the cell.
 * ~return: 1 the cell contains an untracked attacker missile, else 0.
 */
static int check_pixel(int x, int y)
{
    cell_t  cell = env.cell[x][y];

    /* Cell value is >=0 only at the missile position. */
    if (getpixel(screen, x, y) == ATTACKER_COLOR && cell.value >= 0)
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
                /* Assign target index to an untracked attacker missile. */
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

/********************************************************************
 * ENVIRONMENT DRAW
********************************************************************/

/*
 * Draw a legend line (colored rectangle with a text label) on the buffer.
 * 
 * buffer: reference to the buffer to write.
 * spaces: number of positions to skip from the vertical origin.
 * color: color of the rectangle in the legend.
 * label: string containing the label to write beside the rectangle.
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

/*
 * Draw the legend on the buffer.
 * 
 * buffer: reference to the buffer to write.
 */
static void draw_legends(BITMAP *buffer)
{
    draw_legend(buffer, 0, GOAL_COLOR, ": GOAL");

    draw_legend(buffer, 1, WALL_COLOR, ": WALL");

    draw_legend(buffer, 2, ATTACKER_COLOR, ": ATK MISSILE");

    draw_legend(buffer, 3, DEFENDER_COLOR, ": DEF MISSILE");
}

/*
 * Draw the legend on the buffer.
 * 
 * buffer: reference to the buffer to write.
 */
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

/*
 * Draw a missile on the buffer.
 * 
 * buffer: reference to the buffer to write.
 * pos: position of the missile to draw.
 * type: type of the missile to draw.
 */
static void draw_missile(BITMAP *buffer, pos_t pos, missile_type_t type)
{
    int color;

    /* Don't need to draw missile outside borders. */
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

/*
 * Draw a single wall cell in the buffer.
 * 
 * buffer: reference to the buffer to write.
 * pos: position of the cell to draw.
 */
static void draw_wall(pos_t pos, BITMAP *buffer)
{
    putpixel(buffer, pos.x, pos.y, WALL_COLOR);
}

/*
 * Draw a single goal cell in the buffer.
 * 
 * buffer: reference to the buffer to write.
 * pos: position of the cell to draw.
 */
static void draw_goal(pos_t pos, BITMAP *buffer)
{
    putpixel(buffer, pos.x, pos.y, GOAL_COLOR);
}

/*
 * Draw a single cell in the buffer.
 * 
 * buffer: reference to the buffer to write.
 * x: x coordinate of the cell.
 * y: y coordinate of the cell.
 * cell: reference to the cell to draw.
 */
static void draw_cell(cell_t *cell, int x, int y, BITMAP *buffer)
{
    missile_type_t  m_type;
    pos_t           pos;

    pos.x = x;
    pos.y = y;

    if (is_missile_cell(cell))
    {
        m_type = cell_to_missile_type(cell->type);
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

/*
 * Draw the current environment state in the buffer.
 * 
 * buffer: reference to the buffer to write.
 */
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

/*
 * Draw the current buffer on the screen.
 * 
 * buffer: reference to the buffer to copy on screen.
 */
static void draw_buffer_to_screen(BITMAP *buffer)
{
    blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
}

/********************************************************************
 * DISPLAY THREAD
********************************************************************/

/*
 * Display manager task, responsible to write the current environment
 * state on screen on every cycle.
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