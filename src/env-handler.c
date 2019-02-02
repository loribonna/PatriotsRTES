#include "env-handler.h"

env_t env;

cell_t init_cell_empty()
{
    cell_t cell;

    cell.type = EMPTY;
    cell.value = -1;

    return cell;
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
    default:
        safe = 1;
        break;
    }

    return safe;
}

int check_missile_collisions(missile_t *missile)
{
    int ret;
    cell_t cell;

    sem_wait(&env.mutex);

    cell = get_object_type_around(missile->x, missile->y, MISSILE_RADIUS);

    ret = handle_collision_by_cell_type(cell);

    sem_post(&env.mutex);

    return ret;
}

void update_cell_value(int x, int y, int value, cell_type type)
{
    sem_wait(&env.mutex);

    env.cell[x][y].value = value;
    env.cell[x][y].type = type;

    sem_post(&env.mutex);
}

int update_missile_position(int oldx, int oldy, missile_t *missile, int task)
{
    cell_t temp;
    int newx, newy, new_type, safe;

    newx = (int)missile->x;
    newy = (int)missile->y;
    new_type = missile->missile_type == ATTACKER ? ATK_MISSILE : DEF_MISSILE;

    safe = check_missile_collisions(missile);

    if (safe)
    {
        update_cell_value(newx, newy, task, new_type);
    }

    return safe;
}
