#include "missile.h"

int missile_inside_borders(missile_t *missile)
{
    return missile->x >= XWIN ||
           missile->y >= YWIN ||
           missile->x < 0 ||
           missile->y < 0;
}

int create_missile(BITMAP *buffer, missile_t *missile)
{
    int color;

    if (missile_inside_borders(missile))
    {
        return -1;
    }

    if (missile->missile_type == ATTACKER)
    {
        color = ATTACKER_COLOR;
    }
    else
    {
        color = DEFENDER_COLOR;
    }

    circlefill(buffer, missile->x, missile->y, MISSLE_RADIUS, color);

    return 0;
}

int missile_collide(missile_t *missileA, missile_t *missileB)
{
    int distance;

    if (missile_inside_borders(missileA) && missile_inside_borders(missileB))
    {
        distance = get_euclidean_distance(
            missileA->x,
            missileB->x,
            missileA->y,
            missileB->y);

        if (distance <= MISSLE_RADIUS)
        {
            return 1;
        }
    }

    return 0;
}