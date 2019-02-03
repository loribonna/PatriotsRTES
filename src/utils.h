#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include "ptask.h"
#include <assert.h>
#include <math.h>

#define M_PI 3.14159265358979323846
#define DELTA_FACTOR 1000
#define N 4

float frand(float min, float max);

int get_euclidean_distance(float xa, float xb, float ya, float yb);

float get_deltatime(int task_index, int unit);

#ifndef TEST

#define NDEBUG

#endif

#endif