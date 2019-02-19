#ifndef PATRIOTS_H
#define PATRIOTS_H

#include <stdlib.h>

// PI constant used in calculus.
#define M_PI                    3.14159265358979323846
// Division factor for deltatime.
#define DELTA_FACTOR            1000
// Default value indicating no information in an integer variable.
#define NONE                    -1
// Number of nanoseconds in one second, used for conversions of time.
#define NANOSECOND_TO_SECONDS   1000000000.0
// Maximum length of string of text in informative messages.
#define INFO_LEN                150

// Missile radius, used for draw a missile and check collisions.
#define MISSILE_RADIUS          5
// Max number of missile threads -> Size of the queue.
#define N                       4

// Flag used to end all tasks loops.
extern int end;

#endif