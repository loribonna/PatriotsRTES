#include "missile.h"

void init_missile(missile_t *missile)
{
    sem_init(&missile->deleted, 0, 0);
    missile->speed = missile->angle = 0;
    missile->x = missile->y = 0;
    missile->index = -1;
}

int missile_inside_borders(missile_t *missile)
{
    return missile->x >= XWIN ||
           missile->y >= YWIN ||
           missile->x < 0 ||
           missile->y < 0;
}

int draw_missile(BITMAP *buffer, missile_t *missile)
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

    circlefill(buffer, missile->x, missile->y, MISSILE_RADIUS, color);

    return 0;
}

void delete_missile(missile_t *missile)
{
    sem_post(&missile->deleted);
}

int is_deleted(missile_t *missile)
{
    int sval;

    sem_getvalue(&missile->deleted, &sval);

    return sval;
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
        else
        {
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