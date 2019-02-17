#ifndef PATRIOTS_H
#define PATRIOTS_H

// PI constant used in calculus.
#define M_PI            3.14159265358979323846
// Missile radius, used for draw a missile and check collisions.
#define MISSILE_RADIUS 5
// Max number of missile threads -> Size of the queue.
#define N 4
// Division factor for deltatime.
#define DELTA_FACTOR 1000

// Flag used to end all tasks loops.
extern int end;

#endif