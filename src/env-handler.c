#include "env-handler.h"

env_t env;

cell_t init_cell_empty()
{
    cell_t cell;

    cell.type = EMPTY;
    cell.value = -1;

    return cell;
}

// TODO:
void def_point()
{
}

// TODO
void atk_point()
{
}

int handle_collision_by_cell_type(cell_t cell)
{
    int collided;

    collided = 1;

    switch (cell.type)
    {
    case WALL:
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
    int xa, ya, xb, yb, colls;
    missile_type_t missile_type;

    colls = 0;
    missile_type = missile->missile_type;

    xa = missile->x - span >= 0 ? missile->x - span : 0;
    ya = missile->y - span >= 0 ? missile->y - span : 0;

    xb = missile->x + span < XWIN ? missile->x + span : XWIN - 1;
    yb = missile->y + span < YWIN ? missile->y + span : YWIN - 1;

    for (; xa < xb; xa++)
    {
        for (; ya < yb; ya++)
        {
            if (env.cell[xa][ya].type != EMPTY)
            {
                colls += handle_collision(missile_type, env.cell[xa][ya]);
            }
        }
    }

    return colls;
}

int check_missile_collisions(missile_t *missile)
{
    int collisions;
    cell_t cell;

    sem_wait(&env.mutex);

    collisions = handle_collisions_around(missile, MISSILE_RADIUS);

    sem_post(&env.mutex);

    return collisions;
}

void update_cell_value(int x, int y, int value, cell_type type)
{
    sem_wait(&env.mutex);

    env.cell[x][y].value = value;
    env.cell[x][y].type = type;

    sem_post(&env.mutex);
}

int update_missile_position(int oldx, int oldy, missile_t *missile)
{
    int newx, newy, new_type, safe;

    newx = (int)missile->x;
    newy = (int)missile->y;
    new_type = missile->missile_type == ATTACKER ? ATK_MISSILE : DEF_MISSILE;

    safe = check_missile_collisions(missile);

    if (safe)
    {
        update_cell_value(newx, newy, missile->index, new_type);
    }

    return safe;
}
