# PatriotsRTES
Project for Real-Time Embedded Systems course
(UNIMORE, Dipartimento di Ingegneria "Enzo Ferrari").

# Target: Patriots missiles tracker simulation
Simulate a set of Patriot defense missiles that identify enemy targets, predict
their trajectories and are launched to catch them

# Structure

## Threads

The system consists of:
- an attacker launcher thread that spawns attacker missiles randomly;
- a defenging launcher thread that spawns defender missiles, assigning
every defender missile to an attacker missile.

The missiles are threads that computes their position given speed and angle,
for every cycle of the system.

- The attacker missiles are threads that, given a random starting position and 
direction, follows a random path to reach the other side of the screen.
- The defender missiles are threads that must intercept the assigned attacker
missile *before* it reachs its goal. Due to the random path taken
by the attacker, the defender thread must compute and update its direction
on every cycle.

A system cycle performs the following tasks:
- If there is any defender thread, update its position accordingly to the 
assigned attacker missile position. If the assigned missile is reached,
update score and destroy the assigned thread. If the assigned missile has
reached is goal, or for some event no longer exists, exit.
- If there is any attacker thread, update its position. If it has reached its 
goal, update the score and exit.
- Draw the position updates for the missiles involved.

The defender launcher thread runs indipendently, frequently controlling the 
screen in search of new, untracked, attacker missiles. For each enemy point
found controls the environment to check if was already tracked.
If there are untracked attacking missiles and the defending queue is not full,
a new defending missile thread is spawned and the corresponding attacker index
is marked as `tracked`.

An attacker missile is marcked as `tracked` by setting a bit in the missile.

The attacker launcher reacts on a keyboard event, spawning a new attacker
missile if the queue is not full.
The missile spawned has random speed and angle.

## Static parameters

* `N`: Max number of missile threads -> Size of the queue.
This specifies the maximum amount of attacker missiles and defender missiles.
Both attacker and defending queue are handled as FIFO queues.
* `XWIN`: Horizontal size of the window.
* `YWIN`: Vertical size of the window.