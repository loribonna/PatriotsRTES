#include "missile.h"

void init_missile(missile_t *missile)
{
    sem_init(&missile->deleted, 0, 0);
    missile->speed = missile->angle = 0;
    missile->x = missile->y = 0;
    missile->index = -1;
}

int draw_missile(BITMAP *buffer, int x, int y, missile_type_t type)
{
    int color;

    if (!check_borders(x, y))
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

void move_missile(missile_t *missile, float deltatime)
{
    float dx, dy, angle_rad;

    angle_rad = missile->angle * (M_PI / 180);

    dx = missile->speed * cos(angle_rad);
    dy = missile->speed * sin(angle_rad);

    missile->x += dx * deltatime;
    missile->y += dy * deltatime;
}

void print_missile(missile_t *missile)
{
    fprintf(stderr, "i: %i, x: %f, y: %f, angle: %f\n",
            missile->index, missile->x, missile->y, missile->angle);
}