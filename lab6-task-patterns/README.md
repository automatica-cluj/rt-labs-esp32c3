# Lab 6 — Task patterns

The building blocks of a small real-time application, one idea per sketch: a
periodic task on an absolute grid, a state shared under a mutex, an interrupt
that hands its work to a task through a queue, a sporadic task with a minimum
inter-arrival time, an aperiodic task that executes commands, and a state
machine driven by two periodic tasks.

None of the sketches uses `rt.h`: the code is the Arduino API and direct
FreeRTOS calls, so each pattern can be read on its own and reused as it is.
The sketches need only the board on USB and the Serial Monitor: no Wi-Fi and
no MQTT.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours.

Theory behind this lab (concept pages, in Romanian):
[Eliberare periodică](https://github.com/automatica-cluj/rt-concepts/blob/main/1-time/periodic-release.md),
[Modelul de task periodic](https://github.com/automatica-cluj/rt-concepts/blob/main/3-analysis/task-model.md),
[Excludere mutuală](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/mutual-exclusion.md),
[Procesare amânată](https://github.com/automatica-cluj/rt-concepts/blob/main/4-interrupts/deferred-processing.md),
[Debouncing](https://github.com/automatica-cluj/rt-concepts/blob/main/4-interrupts/debouncing.md).

[../docs/freertos_concepts.md](../docs/freertos_concepts.md) introduces tasks,
queues, mutexes and interrupts. Other labs measure some of these effects in
more depth: Lab 1 the sleeps (`periodic_jitter`), Lab 4 the interrupts and
the bounce (`isr_latency`, `button`), Lab 5 the races (`race`).

---

## 1. Setup

Follow [../docs/setup.md](../docs/setup.md). Open the Serial Monitor at
115200 baud, with the line ending set to **New Line** (or **Both NL & CR**).
To send a key or a command, type it in the input line and press Enter.

Only one program can use the board's port: close the Serial Monitor before an
upload if the IDE does not manage to release it by itself.

## 2. A periodic task: `periodic_absolute`

Upload `periodic_absolute/periodic_absolute.ino`. One task has a period
T = 100 ms and a computation time that varies between 5 and 30 ms. Each job
compares its start time s_k with the grid of releases r_k = r_0 + kT. The
difference is the lateness L_k = s_k - r_k. Every 20 jobs the task queues
min L, max L and the jitter J = max L_k - min L_k, and `loop()` prints them,
so that printing cannot disturb the measurement.

### Experiment A — absolute sleep (key `a`)

Press `a`. The task sleeps with `xTaskDelayUntil`.

**Look at** the `L min`, `L max` and `J` columns over several reports, then
the verdict line.

**What it shows.** L stays close to zero however long the jobs take. The
sleep ends at r_{k+1}, an absolute moment on the grid, so the computation
time of a job has no effect on when the next one starts, as long as the job
ends before that moment.

### Experiment B — relative sleep (key `r`)

Press `r`. The task sleeps with `vTaskDelay`: T counted from the end of the
job.

**Look at** `L max` from one report to the next, and the verdict line.

**What it shows.** Each job adds its computation time to the lateness, 17.5 ms
on average, so L grows by roughly 350 ms every 20 jobs and never stops. This
is **drift**: the releases no longer follow the grid. The jitter computed
over 20 jobs is large too, but not because L oscillates: L only grows.

## 3. A shared state: `torn_state`

Upload `torn_state/torn_state.ino`. A writer at priority 2 writes two fields
that must be equal, with some computation between the two writes. A reader at
priority 3, with T = 1 ms, copies them and checks that they are equal. Every
2 s the sketch prints the writes, the reads and the inconsistent states seen.

### Experiment C — with and without the mutex (key `m`)

Let it run with the mutex on, press `m` to switch it off, then `m` again.

**Look at** `inconsistent states` in each mode, and the two verdict lines.

**What it shows.** Without the mutex, many reads find an inconsistent state:
the reader preempts the writer between the two writes. There are so many
because the writer spends almost all its time between them. In a real program
that window is much smaller, so the error is rare, and even harder to find.
With the mutex the count is 0: the reader blocks in `xSemaphoreTake` while the
writer holds the mutex.

Notice how briefly the mutex is held: only while the fields are copied. The
check is done on the local copy, after the mutex is given back. Lab 5 shows
the other way shared data goes wrong, lost updates.

## 4. From an interrupt to a task: `isr_queue`

Upload `isr_queue/isr_queue.ino`. Every falling edge on the BOOT button runs
an interrupt routine that notes the time and puts it in a queue with
`xQueueSendFromISR`. A task takes the elements and prints them. Do not hold
BOOT while pressing RST: that starts the bootloader instead of the sketch.

### Experiment D — presses and bounce (BOOT, key `v`)

Press BOOT a few times, short and long. Then press `v`.

**Look at** the time between consecutive interrupts, the lines marked
`<- bounce?`, and the counts in the verdict line.

**What it shows.** One press often produces several interrupts, a few
milliseconds apart or within the same millisecond: the contacts **bounce**.
The ISR does not filter them. It stays short, uses only the `FromISR`
functions, and leaves the rest to the task. The queue element carries the
data itself, here the time of the interrupt, so nothing depends on when the
task gets to run.

## 5. A sporadic task: `sporadic`

Upload `sporadic/sporadic.ino`. The same queue receives presses from BOOT
(through the ISR) and from the key `b` (from `loop()`, with timeout 0, so
`loop()` never blocks). The task accepts a press only if at least 1000 ms have
passed since the last accepted one, and each accepted press toggles the LED.

### Experiment E — the minimum inter-arrival time (BOOT, keys `b`, `v`)

Press BOOT rarely, then often. Type `bbbb` and press Enter. Press `v`.

**Look at** the `ACCEPTED` and `REJECTED` lines, the time printed in each
`REJECTED` line, and the counts in the verdict line.

**What it shows.** Presses that come too soon are rejected, and so are the
extra interrupts from bounce, with no filter in the ISR. Of `bbbb`, four
presses that arrive together, only the first is accepted. The arrival time is
put in the queue element by whoever sends it, not read by the task when it
takes the element: otherwise the task would measure when it got round to the
press, not when the press arrived.

## 6. An aperiodic task: `aperiodic`

Upload `aperiodic/aperiodic.ino`. The commands `LED ON`, `LED OFF`,
`BLINK <ms>` and `STATUS` go through a queue as text: the queue element holds
the `char` array, not a pointer, because `loop()` reuses its buffer for the
next line. Each command releases one job of the aperiodic task, which changes
the LED state through `led_set()`, holding the mutex only while it writes the
fields, and answers with `OK ...` or `ERROR ...`. A periodic task with
T = 50 ms copies the state and applies it to the LED.

### Experiment F — commands, right and wrong (`LED ON`, `BLINK 200`, `STATUS`)

Send `LED ON`, `BLINK 200`, `LED OFF`, `STATUS`. Then send wrong commands:
`BLINK`, `BLINK 20`, `BLINK abc`, `led on`, `xyz`, and `STATUS` again.

**Look at** the LED, the `OK` and `ERROR` answers, and the counts in the
verdict line printed after each `STATUS`.

**What it shows.** Each wrong command gets an `ERROR` answer and the task goes
on: bad input must not stop an aperiodic task. Commands are case sensitive, so
`led on` is an unknown command. The two tasks run at different rates and meet
only in the state protected by the mutex.

## 7. A state machine and two periodic tasks: `fsm_periodic`

Upload `fsm_periodic/fsm_periodic.ino`. A parking barrier goes through
`CLOSED`, `OPENING` (3 s), `OPEN` (5 s) and `CLOSING` (3 s).

The **control** task (T = 100 ms) decides the state, and the time spent in
each state is measured with `millis()` from the moment it was entered. The
**output** task (T = 500 ms) copies the state and decides what is seen: the
LED, which blinks in `OPENING` and `CLOSING` (lit in one job, off in the next),
and the seconds left, rounded up (0 in `CLOSED`, which has no duration).

### Experiment G — one cycle (key `m`)

Press `m` while the barrier is closed, and wait for it to close again.

**Look at** the `[control]` transitions, the `left` column and the LED, then
the verdict line.

**What it shows.** There are four transitions, and the cycle takes about
11 s. A transition can only happen at a release of the control task, so
each state lasts its nominal time rounded up to the 100 ms grid. The state
machine decides, the output task shows: the output has a slower period, and
it never changes the state.

### Experiment H — a request while the barrier is up (key `m`, twice)

Press `m`, then press `m` again while the barrier is `OPEN` or `CLOSING`.

**Look at** the `request` column, and what the barrier does after it closes.

**What it shows.** The request is cleared when the barrier reaches `OPEN`,
the moment the car can pass. A request that comes after that stays recorded,
and the barrier rises again after it comes down: an event is not lost just
because the machine was busy in another state.
