#include "env-handler.h"

env_t env;

cell_t init_cell_empty()
{
    cell_t cell;

    cell.type = EMPTY;
    cell.value = EMPTY_CELL;

    return cell;
}

cell_t init_wall_cell()
{
    cell_t cell;

    cell.type = WALL;
    cell.value = OTHER_CELL;

    return cell;
}

cell_t init_goal_cell()
{
    cell_t cell;

    cell.type = GOAL;
    cell.value = OTHER_CELL;

    return cell;
}

int wall_init_check(int x, int y)
{
    return x < WALL_THICKNESS ||
           y < WALL_THICKNESS ||
           XWIN - x < WALL_THICKNESS ||
           YWIN - y < WALL_THICKNESS;
}

int goal_init_check(int x, int y)
{
    return y > GOAL_START_Y;
}

void init_cell(int x, int y)
{
    if (wall_init_check(x, y))
    {
        env.cell[x][y] = init_wall_cell();
    }
    else if (goal_init_check(x, y))
    {
        env.cell[x][y] = init_goal_cell();
    }
    else
    {
        env.cell[x][y] = init_cell_empty();
    }
}

void init_env()
{
    int x, y;

    for (x = 0; x < XWIN; x++)
    {
        for (y = 0; y < YWIN; y++)
        {
            init_cell(x, y);
        }
    }
}

cell_t get_object_type_around(int x, int y, int span)
{
    int xa, ya, xb, yb;

    xa = x - span >= 0 ? x - span : 0;
    ya = y - span >= 0 ? y - span : 0;

    xb = x + span < XWIN ? x + span : XWIN - 1;
    yb = y + span < YWIN ? y + span : YWIN - 1;

    for (; xa < xb; xa++)
    {
        for (; ya < yb; ya++)
        {
            if (env.cell[x][y].type != EMPTY)
            {
                return env.cell[xa][ya];
            }
        }
    }

    return init_cell_empty();
}

int is_empty_cell(cell_t cell)
{
    return (cell.type == EMPTY) ||
           (cell.value == EMPTY_CELL);
}

int is_missile_cell(cell_t cell)
{
    return !is_empty_cell(cell) &&
           (cell.type == ATK_MISSILE || cell.type == DEF_MISSILE);
}

int is_wall_cell(cell_t cell)
{
    return !is_empty_cell(cell) &&
           cell.type == WALL &&
           cell.value == OTHER_CELL;
}

int is_goal_cell(cell_t cell)
{
    return !is_empty_cell(cell) &&
           cell.type == GOAL &&
           cell.value == OTHER_CELL;
}

int handle_collision_by_cell_type(cell_t cell)
{
    int safe;

    switch (cell.type)
    {
    case WALL:
        safe = 0;
        break;
    case DEF_MISSILE:
        safe = 0;
        def_missile_hit(cell.value);
        break;
    case ATK_MISSILE:
        safe = 0;
        atk_missile_hit(cell.value);
        break;
    case GOAL:
        safe = 0;
        atk_missile_goal(cell.value);
        break;
    default:
        safe = 1;
        break;
    }

    return safe;
}

int handle_missile_collisions(missile_t *missile)
{
    int ret;
    cell_t cell;

    cell = get_object_type_around(missile->x, missile->y, MISSILE_RADIUS);

    ret = handle_collision_by_cell_type(cell);

    return ret;
}

void update_cell_value(int x, int y, int value, cell_type type)
{
    env.cell[x][y].value = value;
    env.cell[x][y].type = type;
}

int update_missile_position(int oldx, int oldy, missile_t *missile, int task)
{
    int newx, newy, new_type, safe;

    newx = (int)missile->x;
    newy = (int)missile->y;
    new_type = missile->missile_type == ATTACKER ? ATK_MISSILE : DEF_MISSILE;

    sem_wait(&env.mutex);

    safe = handle_missile_collisions(missile);

    if (safe)
    {
        update_cell_value(newx, newy, task, new_type);
        env.cell[oldx][oldy] = init_cell_empty();
    }

    sem_post(&env.mutex);

    return safe;
}

void draw_cell(cell_t cell, int x, int y, BITMAP *buffer)
{
    if (is_missile_cell(cell))
    {
        draw_missile(buffer, x, y, cell.type);
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
            if (!is_empty_cell(env.cell[x][y]))
            {
                draw_cell(env.cell[x][y], x, y, buffer);
            }
        }
    }

    sem_post(&env.mutex);
}
