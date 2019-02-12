#include "env-handler.h"

env_t env;

void init_cell_empty(cell_t *cell)
{
    cell->type = EMPTY;
    cell->value = EMPTY_CELL;
}

void init_wall_cell(cell_t *cell)
{
    cell->type = WALL;
    cell->value = OTHER_CELL;
}

void init_goal_cell(cell_t *cell)
{
    cell->type = GOAL;
    cell->value = OTHER_CELL;
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

void init_env()
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

void def_point()
{
    env.def_points++;
}

void atk_point()
{
    env.atk_points++;
}

int handle_collision_by_cell_type(cell_t cell)
{
    int collided;

    collided = 1;

    switch (cell.type)
    {
    case WALL:
        break;
    case GOAL:
        break;
    case DEF_MISSILE:
        delete_def_missile(cell.value);
        break;
    case ATK_MISSILE:
        delete_atk_missile(cell.value);
        break;
    default:
        collided = 0;
        break;
    }

    return collided;
}

int handle_collision(missile_type_t missile_type, cell_t cell)
{
    int collided;

    collided = handle_collision_by_cell_type(cell);

    if (collided)
    {
        if (
            (missile_type == ATTACKER && cell.type == DEF_MISSILE) ||
            (missile_type == DEFENDER && cell.type == ATK_MISSILE))
        {
            def_point();
        }
        if (missile_type == ATTACKER && cell.type == GOAL)
        {
            atk_point();
        }
    }

    return collided;
}

int handle_collisions_around(missile_t *missile, int span)
{
    int xa, ya, xb, yb, y, x, colls;
    missile_type_t missile_type;

    colls = !check_borders(missile->x, missile->y);
    missile_type = missile->missile_type;

    xa = missile->x - span >= 0 ? missile->x - span : 0;
    ya = missile->y - span >= 0 ? missile->y - span : 0;
    xb = missile->x + span < XWIN ? missile->x + span : XWIN;
    yb = missile->y + span < YWIN ? missile->y + span : YWIN;

    for (x = xa; x < xb; x++)
    {
        for (y = ya; y < yb; y++)
        {
            if (x != missile->x && y != missile->y && env.cell[x][y].type != EMPTY)
            {
                colls += handle_collision(missile_type, env.cell[x][y]);
                fprintf(stderr, "%i\n", colls);
            }
        }
    }

    return colls;
}

void update_cell_value(int x, int y, int value, cell_type type)
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

    collisions = handle_collisions_around(missile, MISSILE_RADIUS);

    if (!collisions)
    {
        update_cell_value(newx, newy, missile->index, type);
    }

    sem_post(&env.mutex);

    return collisions;
}

void draw_cell(cell_t cell, int x, int y, BITMAP *buffer)
{
    missile_type_t m_type;

    if (is_missile_cell(cell))
    {
        m_type = cell.type == ATK_MISSILE ? ATTACKER : DEFENDER;
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
            if (!is_empty_cell(env.cell[x][y]))
            {
                draw_cell(env.cell[x][y], x, y, buffer);
            }
        }
    }

    draw_labels(buffer, env.atk_points, env.def_points);

    sem_post(&env.mutex);
}
