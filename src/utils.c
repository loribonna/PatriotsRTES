#include "utils.h"

float frand(float min, float max)
{
    float r;

    r = rand() / (float)RAND_MAX;
    return min + (max - min) * r;
}

int get_euclidean_distance(float xa, float xb, float ya, float yb)
{
    return sqrt(pow(xa - xb, 2) + pow(ya - yb, 2));
}

float get_deltatime(int task_index) {
    return (float)ptask_get_period(task_index, MILLI)/DELTA_FACTOR;
}