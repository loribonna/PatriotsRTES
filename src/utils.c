#include "utils.h"

float frand(float min, float max)
{
    float r;

    r = rand() / (float)RAND_MAX;
    return min + (max - min) * r;
}

int get_euclidean_distance(int xa, int ya, int xb, int yb)
{
    return sqrt(pow(xa - xb, 2) + pow(ya - yb, 2));
}