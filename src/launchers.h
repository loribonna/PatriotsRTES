/********************************************************************
 * Lorenzo Bonicelli 2019
 * 
 * This file contains the declarations of the structures used to
 * control missile tasks and function prototypes necessary to create
 * and update such tasks.
 * 
********************************************************************/

#ifndef LAUNCHERS_H
#define LAUNCHERS_H

#include <stdlib.h>
#include <semaphore.h>

#include "patriots.h"

/********************************************************************
 * ATTACK PARAMETERS
********************************************************************/

// Upper extremity for random attack missile speed.
#define MAX_ATK_SPEED 100
// Lower extremity for random attack missile speed.
#define MIN_ATK_SPEED ((int)(MAX_ATK_SPEED * 0.3))
// Maximum trajectory angle for random attack missile.
#define MAX_ATK_ANGLE 30
// Delay between subsequent attack missile launches.
#define ATK_SLEEP_DELAY 500 * 1000 * 1000 // 500 milliseconds

// Period of the attack launcher task.
#define ATK_LAUNCHER_PERIOD 60
// Priority of the attack launcher task.
#define ATK_LAUNCHER_PRIO 1
// Priority of the attack missile task.
#define ATK_MISSILE_PRIO 2
// Period of the attack missile task.
#define ATK_MISSILE_PERIOD 10
// Relative deadline of the attacker missile task.
#define ATK_MISSILE_DEADLINE (ATK_MISSILE_PERIOD)

/********************************************************************
 * DEFENDER PARAMETERS
********************************************************************/

// Common starting point for defender missiles.
#define DEF_MISSILE_START_Y (GOAL_START_Y - MISSILE_RADIUS - 1)
// Fixed defender missile speed.
#define DEF_MISSILE_SPEED 130
// Delay between subsequent defender missile launches.
#define DEF_SLEEP_DELAY 50 * 1000 * 1000 // 50 milliseconds

// Period of the defender launcher task.
#define DEF_LAUNCHER_PERIOD 40
// Priority of the defender launcher task.
#define DEF_LAUNCHER_PRIO 1
// Priority of the defender missile task.
#define DEF_MISSILE_PRIO 2
// Period of the defender missile task.
#define DEF_MISSILE_PERIOD 15
// Relative deadline of the defender missile task.
#define DEF_MISSILE_DEADLINE (DEF_MISSILE_PERIOD)

// Precision used in trajectory calculation (percentual)
#define TRAJECTORY_PRECISION 50
// Ubber extremity to limit trajectory calculation loop.
#define SAMPLE_LIMIT (2 * TRAJECTORY_PRECISION)
// Minimum number of samples to collect to calculate the trajectory
// of an attacker missile.
#define MIN_SAMPLES (SAMPLE_LIMIT / 5)
// Level of precision used in trajectory calculus.
#define EPSILON (1 / TRAJECTORY_PRECISION)

// Type of missile
typedef enum
{
    ATTACKER,
    DEFENDER
}   missile_type_t;

// Private semaphore base structure.
typedef struct
{
    sem_t   sem;
    int     count;  // Number of active threads.
    int     blk;    // Number of blocked threads.
}   private_sem_t;

// Single missile structure.
typedef struct
{
    int             x, y;                   // Position in the screen.
    float           partial_x, partial_y;   // Partial position for computation.
    float           angle;                  // Current angle of the missile.
    float           speed;                  // Current speed of the missile.
    int             index;                  // Index in the belonging queue.
    int             deleted;                // Flag to delete a missile.
    int             assigned_target;        // Index assigned if discoveded.
    sem_t           mutex;                  // Mutex for the structure.
    missile_type_t  missile_type;           // Type of missile.
}   missile_t;

// Fifo queue gestor.
typedef struct
{
    int             freeIndex;  // Index of the next free element.
    int             headIndex;  // Index of the next used element.
    int             tailIndex;  // Index of the previus free element
    int             next[N];    // Queue of available indexes.
    sem_t           mutex;      // Mutex for the structure.
    private_sem_t   write_sem;  // Private semaphore to extract a free element.
    private_sem_t   read_sem;   // Private semaphore to extract a used element.
}   fifo_queue_gestor_t;

// Single missile queue gestor.
typedef struct
{
    missile_t           queue[N];
    fifo_queue_gestor_t gestor;
}   missile_gestor_t;

// Trajectory of a missile, used to compute defender starting point.
typedef struct
{
    float   m;          // Angular coefficient of the trajectory.
    float   b;          // Vertical origin of the tracjectory.
    float   speed;      // Speed of the missile following the trajectory.
}   trajectory_t;

/*
 * Initialize attacker and defender launchers.
 */
void init_launchers();

/*
 * Initialize a private semaphore structure.
 * 
 * private_sem_t: reference to the private semaphore to initialize.
 */
void init_private_sem(private_sem_t *p_sem);

/*
 * Launch attack launcher task.
 */
void launch_atk_launcher();

/*
 * Launch defender launcher task.
 */
void launch_def_launcher();

/*
 * BLOCKING: Request an attacker missile task launch.
 * Block if there are tasks to wake up.
 */
void request_atk_launch();

/*
 * Delete an attacker missile.
 * 
 * index: index of the missile to delete.
 */
void delete_atk_missile(int index);

/*
 * Delete an defender missile.
 * 
 * index: index of the missile to delete.
 */
void delete_def_missile(int index);

/*
 * Assign a target index to an attacker missile task.
 * 
 * index: index of the attacker missile.
 * target: target index to assign.
 */
void assign_target_to_atk(int index, int target);

/*
 * Check if a given target index is already assigned to a 
 * defender missile task.
 * 
 * target: index to check.
 * ~return: 1 if the given index is already tracked, else 0. 
 */
int is_already_tracked(int target);

#endif