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
- a defender launcher thread that spawns defender missiles, assigning
every defender missile to an attacker missile.

The missiles are threads that computes their position given speed and angle,
for every cycle of the system.

- The attacker missiles are threads that, given a random starting position and 
angle, follows a straight direction to reach the other side of the screen.
- The defender missiles are threads that must intercept the assigned attacker
missile *before* it reachs its goal. The direction is determined before the missile
starts moving, after analyzing the assigned attacker's direction and speed.

In the current version, the defender missile follows a straight line with an angle 
of 90 degrees to intercept the attacker. Following versions will have the attacker
missile following a random path and the defender thread will have to compute 
and update its direction on every cycle.

A system cycle performs the following tasks:
- If there is any defender thread, update its position accordingly to the 
assigned attacker missile position. If the assigned missile is reached,
update score and destroy the assigned thread. If the assigned missile has
reached is goal, or for some event no longer exists, exit.
- If there is any attacker thread, update its position. If it has reached its 
goal, update the score and exit.
- Draw the position updates for the missiles involved.

The defender launcher thread runs indipendently, controlling the screen in search 
of new, untracked, attacker missiles. For each enemy point found controls the 
environment to check if was already tracked. If there are untracked attacking 
missiles and the defending queue is not full, a new defending missile thread 
is spawned and the corresponding attacker index is marked as `tracked`.

An attacker missile is marked as `tracked` by assigning an index that will be
used to check its position by the defender missile.

The attacker launcher reacts on a keyboard event, spawning a new attacker
missile if the queue is not full. The missile spawned has random speed and angle.

## Static parameters

### System parameters

* `N`: Max number of missile threads -> Size of the queue.
Specifies the maximum amount of attacker missiles and defender missiles.
Both attacker and defending queue are handled as FIFO queues.
* `M_PI`: PI constant used in calculus.
* `DELTA_FACTOR`: Division factor for deltatime.
* `MISSILE_RADIUS`: Missile radius, used for draw a missile and check collisions.

### Thread parameters

* **Display manager**
    * `DISPLAY_PRIO`: Priority of the display manager task.
    * `DISPLAY_PERIOD`: Period of the display manager task. Calculated from `REFRESH_RATE`.
* **Attack launcher**
    * `ATK_LAUNCHER_PRIO`: Priority of the attack launcher task.
    * `ATK_LAUNCHER_PERIOD`: Period of the attack launcher task.
* **Attack missile**
    * `ATK_MISSILE_PRIO`: Priority of the attack missile task.
    * `ATK_MISSILE_PERIOD`: Period of the attack missile task.
* **Defender launcher**
    * `DEF_LAUNCHER_PRIO`: Priority of the defender launcher task.
    * `DEF_LAUNCHER_PERIOD`: Period of the defender launcher task.
* **Defender missile**
    * `DEF_MISSILE_PRIO`: Priority of the defender missile task.
    * `DEF_MISSILE_PERIOD`: Period of the attack missile task.

### Display parameters

* `REFRESH_RATE`: Refresh rate of the screen, used to compute display period.
* **Window parameters**
    * `XWIN`: Horizontal size of the window.
    * `YWIN`: Vertical size of the window.
    * `WALL_THICKNESS`: Thickness of the wall around the window.
    * `GOAL_START_Y`: Starting point of the goal for attacker missile.
* **Colors**
    * `BKG_COLOR`: Background color of the display.
    * `WALL_COLOR`: Color of the wall around the window.
    * `GOAL_COLOR`: Color of the goal for the attacker missile.
    * `LABEL_COLOR`: Color of the text labels.
    * `ATTACKER_COLOR`: Color of the attaker missile.
    * `DEFENDER_COLOR`: Color of the defender missile.
* **Legend and labels parameters**
    * `LABEL_LEN`: Maximum length of string of text in labels.
    * `RECT_H`: Height of the rectangle used in the legend.
    * `RECT_W`: Width of the rectangle used in the legend.
    * `LEGEND_X`: Horizontal starting point of the legend.
    * `LEGEND_Y`: Vertical starting point of the legend.
    * `LABEL_H`: Height of the text labels, used to calculate vertical starting point.
    * `LABEL_X`: Horizontal starting point of the text labels.
    * `GET_Y_LABEL(s)`: Get vertical starting point given the number of spaces `s`.
    * `SPACING`: Spaces between lines in the legend.

### Environment parameters

* `ENV_PRIOS`: Number of priorities for environment access. The priority of access can be: low (`LOW_ENV_PRIO`), medium (`MIDDLE_ENV_PRIO`) or high (`HIGH_ENV_PRIO`).
* `EMPTY_CELL`: Value of an empty cell inside the environment.
* `OTHER_CELL`: Value of a cell that is not empty but contains static data (wall or goal cell).

### Attacker parameters

* `MAX_ATK_SPEED`: Upper extremity for random attack missile speed. 
* `MIN_ATK_SPEED`: Lower extremity for random attack missile speed. Computed from `MAX_ATK_SPEED`.
* `MAX_ATK_ANGLE`: Maximum trajectory angle for random attack missile.
* `ATK_SLEEP_DELAY`: Delay between subsequent attack missile launches.

### Defender paramenters

* `DEF_MISSILE_START_Y`: Common starting point for defender missiles. Calculated from `GOAL_START_Y` and `MISSILE_RADIUS`.
* `DEF_MISSILE_SPEED`: Fixed defender missile speed.
* `DEF_SLEEP_DELAY`: Delay between subsequent defender missile launches.
* `TRAJECTORY_PRECISION`: Precision used in trajectory calculation (percentual).
* `SAMPLE_LIMIT`: Ubber extremity to limit trajectory calculation loop. Calculated from `TRAJECTORY_PRECISION`.
* `MIN_SAMPLES`: Minimum number of samples to collect to calculate the trajectory of an attacker missile. Calculated from `TRAJECTORY_PRECISION`.
* `EPSILON`: Level of precision used in trajectory calculus. Calculated from `TRAJECTORY_PRECISION`.