#include "missile.h"

int missile_inside_borders(missile_t *missile)
{
    return check_borders(missile->x, missile->y);
}

int draw_missile(BITMAP *buffer, int x, int y, missile_type_t type)
{
    int color;

    if (check_borders(x, y))
    {
        return -1;
    }

    if (type == ATTACKER)
    {
        color = ATTACKER_COLOR;
    }
    else
    {
        color = DEFENDER_COLOR;
    }

    circlefill(buffer, x, y, MISSILE_RADIUS, color);

    return 0;
}

int missiles_collide(missile_t *missileA, missile_t *missileB)
{
    int distance;

    if (missile_inside_borders(missileA) && missile_inside_borders(missileB))
    {
        distance = get_euclidean_distance(
            missileA->x,
            missileB->x,
            missileA->y,
            missileB->y);

        if (distance <= MISSILE_RADIUS)
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

    return 0;
}

void move_missile(missile_t *missile, float deltatime)
{
    float dx, dy;

    dx = missile->speed * cos(missile->angle);
    dy = missile->speed * sin(missile->angle);

    missile->x += dx * deltatime;
    missile->y += dy * deltatime;
}