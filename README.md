# PatriotsRTES
Project for Real-Time Embedded Systems course
(UNIMORE, Dipartimento di Ingegneria "Enzo Ferrari").

# Target: Patriots missiles tracker simulation
Simulate a set of Patriot defense missiles that identify enemy targets, predict
their trajectories and are launched to catch them

# User interface

The commands available for the user are:
- `space`: generate an attacker missile if the current number is not over limit.
- `esc`: end the program.

## Low performance environments

In order to utilize the software under low performance environments it was 
introduced the `TIME_SCALE` MACRO (with default 1.5). This parameter is used to 
scale the deltatime (to calculate movements) in order to use higher periods for
missiles.  
If this time dilation factor is set larger, the missiles will take more time to 
perform a movement with respect to the space. 

To make the changes to the time scaling factor effective is necessary to 
rebuild the program, with the commands presented in the following section.

## Build and run PATRIOTS

Use `make run` to check if display mode is available, compile and run the 
application.  
Alternatively, use `make` (or `make build`) to compile and then run the built
executable created in `/build` as superuser with `sudo ./build/patriots` 
(hypotizing to be in the base folder).
Other available commands for `make` are:
- `make clean`: clean the build directory.
- `make all`: combine `clean` and `build` operations. This is also the default
command executed with `make` (with no arguments).
- `make check-env`: check if display is supported. This is just a basic control
to avoid to run the program in environments that don't have display support. 
This is only a necessary, but not sufficient, condition in order to enable the
graphical environment.
- `make compile`: compile the `.c` files and generate the corresponding `.o` in
the `/build` directory.
- `make link`: combine the `.o` files in the `/build` directory and generate 
the executable file, always in the `/build` directory.  
The command `make install` is not available.

In order to use docker it is necessary to build the image, using the provided
`Dockerfile`. Although it is possible to build the image, in order to run the 
the newly created container is then necessary to run it interactively and with 
GUI support.  
It is worth mention that the developer that created the system did not manage 
to run the application as a GUI docker application, so the application does not 
feature any support for that.

# Structure

The project uses the library [ptask](https://github.com/glipari/ptask).  
In order to succesfully spawn a task using the functions provided, the program 
must be run as superuser.  
The ptask library provides access to an interface for aperiodic and periodic
tasks, built on top of the *pthread* library.  

## Modules

The projects consists of 3 modules:
- `patriots`: contains the `main` function. Performs the initialization of the
system and spawns the launcher and display tasks and then checks for a keyboard
event.
- `gestor`: contains the environment management and display functions. The 
environment's main purpose is to keep the state of the data displayed and 
permit to have a common container for the position of all entities on the 
screen. This allows to check for collisions precisely and efficiently.
- `launchers`: contains the functions necessary to create and manage the 
movement of the missiles. It also contains the fifo-queue managers for the
attacker and defender queues.

## Tasks

The system consists of:
- an attacker launcher task that spawns attacker missiles with random initial
direction. The launcher waits to be awaken from the main thread, which reacts 
to the pressing of the key `space`.
- a defender launcher task that spawns defender missiles, assigning
every defender missile to an attacker missile.
- several (limited by `N`) attacker missile tasks spawned by the attacker 
launcher.
- several (limited by `N`) defender missile tasks spawned by the defender 
launcher in order to intercept the attacker missile assigned (target).
- a display task that draw every missile and static parts of the screen on 
every cycle. The current state of the application is contained in the 
environment (`env`).  
Only the missile and display tasks have a deadline. The Launcher tasks does not
have one due to the long cycles of wait are subject to.  
The cycle ends if the `end` flag is set by the main.

The missiles are tasks that computes their position given speed and angle,
for every cycle of the system.  
- The attacker missiles are tasks that, given a random starting position and 
angle, follows a straight direction to reach the other side of the screen.
- The defender missiles are tasks that must intercept the assigned attacker
missile *before* it reachs its goal. The direction is determined before the 
missile starts moving, after analyzing the assigned attacker's direction and 
speed.

In the current version, the defender missile follows a straight line with an 
angle of 90 degrees to intercept the attacker. Following versions will have 
the attacker missile following a random path and the defender task will have 
to compute and update its direction on every cycle.

The defender launcher task scans the screen in search of new, untracked, 
attacker missiles. For each enemy point found controls the environment to 
check if was already tracked. If there are untracked attacking missiles and, 
if the defending queue is not full, a new defending missile task is spawned 
and the corresponding attacker index is marked as `tracked`.  
An attacker missile is marked as `tracked` by assigning an index that will be
used to check its position by the defender missile.

## Details about trajectory prediction

To search for the optimal horizontal starting point, in order to intercept the 
attacker missile, the defender missile must compute speed and direction of the 
target. This is done by the `collect_positions` function. To take account of 
the possible overheads and suspensions of the tasks, it is necessary to use 
the absolute time of the machine, provided by `clock_gettime` using the clock
`CLOCK_MONOTONIC`.  
In order to have an acceptable level of precision, it is necessary to repeat 
the scan of the target multiple times, until the gap between multiple mesured 
speeds is low enough.

The x coordinate of the starting position is then calculated by bisection by 
the function `get_expected_position_x`.  
The algoritm needs to take account of the direction of the target in order to 
initialize correctly the parameters. If the angular coefficient of the target 
is greater of 0, the interception point will be to the right (because the 
screen is inverted vertically) with respect to the last position and viceversa.  
In the algoritm the suffix 'b' is related to the defender trajectory and the
suffix 'a' to the attacker trajectory.

The static parameters of the algoritm are:
 * `DEF_MISSILE_START_Y`: vertical starting point the defender missiles.
 * `DEF_MISSILE_SPEED`: speed of the defender missiles.
 * `SAMPLE_LIMIT`: upper bound for the calculation, to avoid infinite loop.
 * `EPSILON`: level of precision required.
 * `TIME_SCALE`: time scaling factor for high period tasks.

The main parameters of the algoritm are:
 * `x_min`: starting point of the horizontal window where the intercept can be.
 * `x_max`: ending point of the horizontal window where the intercept can be.
 * `speed_a`: estimated speed of the target missile.
 * `dsb`: euclidean distance from the defender starting point and the intercept. 
 Because the defender trajectory is straight at 90 degrees, this is equal to 
 the difference of the vertical coordinates of the points.
 * `dsa`: euclidean distance from the attacker's last position and the 
 intercept, calculated using the current coordinates of the intercept.
 * `tmp_dsa`: distance from the attacker's last position and the intercept,
 calculated from `dsb`. Given the speed of the target (`speed_a`) and the 
 constant speed of the defender missile (`DEF_MISSILE_SPEED`), the distance 
 between the intercept and the defender starting point (`dsb`) and the 
 intercept and the target last position (`dsa`) is: 
 `dsa = (speed_a / DEF_MISSILE_SPEED) * dsb`. This is because the deltatime 
 needed to reach the intercept from the perspective of the defender and the 
 target must be equal.

The algoritm performs the following steps:
 * Initialization of the parameters `x_min` and `x_max` based on the angular
 coefficient of the trajectory and the last position of the target.
 * Repeat until convergence:
    * Take a coordinate `x` in the middle of `x_min` and `x_max` and the 
    relative `y` coordinate based on the target trajectory. The couple `(x, y)` 
    are the current expected interception point.
    * Calculate `dsb` and `dsa` directly from the current interception point.
    * Calculate `tmp_dsa` using the relation with `dsb`.
    * Update `x_min` and `x_max`:
        * If `tmp_dsa` is less than `dsa` it is necessary to raise `dsb`, so 
        the current `x` value of the intercept must be assigned to `x_min`. 
        Viceversa, if `tmp_dsa` is greater than `dsa` it is necessary to lower 
        `dsb`, which means assign the `x` value of the intercept to `x_max`.  
        The former scenario refers to the case where the angular coefficient 
        `m` of the target is less than 0. For the other scenario it is 
        sufficient to swap the conditions.
    * Calculate the error of the prediction by taking the absolute value of the
    difference between `tmp_dsa` and `dsa`.  
    The algoritm converges if the error is less than `EPSILON`.

## Static parameters

### System parameters

* `N`: Max number of missile tasks -> Size of the queue.
Specifies the maximum amount of attacker missiles and defender missiles.
Both attacker and defending queue are handled as FIFO queues.
* `M_PI`: PI constant used in calculus.
* `DELTA_FACTOR`: Division factor for deltatime.
* `MISSILE_RADIUS`: Missile radius, used for draw a missile and check 
collisions.
* `NONE`: Default value indicating no information in an integer variable.
* `NANOSECOND_TO_SECONDS`: Number of nanoseconds in one second, used for 
conversions of time.
* `INFO_LEN`: Maximum length of string of text in informative messages.

The `N` and `MISSILE_RADIUS` should be the only parameters that the user can
change.

### Tasks parameters

* **Display manager**
    * `DISPLAY_PRIO`: Priority of the display manager task.
    * `DISPLAY_PERIOD`: Period of the display manager task. Calculated from 
    `REFRESH_RATE`.
    * `DISPLAY_DEADLINE`: Relative deadline of the display manager task, set
    equal to `DISPLAY_PERIOD`.
* **Attack launcher**
    * `ATK_LAUNCHER_PRIO`: Priority of the attack launcher task.
    * `ATK_LAUNCHER_PERIOD`: Period of the attack launcher task.
* **Attack missile**
    * `ATK_MISSILE_PRIO`: Priority of the attack missile task.
    * `ATK_MISSILE_PERIOD`: Period of the attack missile task.
    * `ATK_MISSILE_DEADLINE`: Relative deadline of the attacker missile task, set
    equal to `ATK_MISSILE_PERIOD`.
* **Defender launcher**
    * `DEF_LAUNCHER_PRIO`: Priority of the defender launcher task.
    * `DEF_LAUNCHER_PERIOD`: Period of the defender launcher task.
* **Defender missile**
    * `DEF_MISSILE_PRIO`: Priority of the defender missile task.
    * `DEF_MISSILE_PERIOD`: Period of the attack missile task.
    * `DEF_MISSILE_DEADLINE`: Relative deadline of the defender missile task, set
    equal to `DEF_MISSILE_PERIOD`.

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
    * `TUTORIAL_Y`: Vertical starting point of the tutorial text.
    * `LABEL_LEN`: Maximum length of string of text in labels.
    * `RECT_H`: Height of the rectangle used in the legend.
    * `RECT_W`: Width of the rectangle used in the legend.
    * `LEGEND_X`: Horizontal starting point of the legend.
    * `LEGEND_Y`: Vertical starting point of the legend, calculated from
    `TUTORIAL_Y`.
    * `LABEL_H`: Height of the text labels, used to calculate vertical starting 
    point.
    * `LABEL_X`: Horizontal starting point of the text labels.
    * `GET_Y_LABEL(s)`: Get vertical starting point given the number of spaces 
    `s`.
    * `SPACING`: Spaces between lines in the legend.

### Environment parameters

* `ENV_PRIOS`: Number of priorities for environment access. The priority of 
access can be: low (`LOW_ENV_PRIO`), medium (`MIDDLE_ENV_PRIO`) or high 
(`HIGH_ENV_PRIO`).
* `EMPTY_CELL`: Value of an empty cell inside the environment.
* `OTHER_CELL`: Value of a cell that is not empty but contains static data 
(wall or goal cell).

### Attacker parameters

* `MAX_ATK_SPEED`: Upper extremity for random attack missile speed. 
* `MIN_ATK_SPEED`: Lower extremity for random attack missile speed. Computed 
from `MAX_ATK_SPEED`.
* `MAX_ATK_ANGLE`: Maximum trajectory angle for random attack missile.
* `ATK_SLEEP_DELAY`: Delay between subsequent attack missile launches.

### Defender paramenters

* `DEF_MISSILE_START_Y`: Common starting point for defender missiles. 
Calculated from `GOAL_START_Y` and `MISSILE_RADIUS`.
* `DEF_MISSILE_SPEED`: Fixed defender missile speed.
* `DEF_SLEEP_DELAY`: Delay between subsequent defender missile launches.
* `TRAJECTORY_PRECISION`: Precision used in trajectory calculation (percentual).
* `SAMPLE_LIMIT`: Ubber extremity to limit trajectory calculation loop. 
Calculated from `TRAJECTORY_PRECISION`.
* `MIN_SAMPLES`: Minimum number of samples to collect to calculate the 
trajectory of an attacker missile. Calculated from `TRAJECTORY_PRECISION`.
* `EPSILON`: Level of precision used in trajectory calculus. Calculated 
from `TRAJECTORY_PRECISION`.