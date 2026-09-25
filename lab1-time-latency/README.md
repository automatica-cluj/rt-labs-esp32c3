# Lab 1 — Time, latency and jitter

Get the board running, see what the clocks on it can measure, and watch how
regular a periodic task really is: relative against absolute sleeps, the
effect of priorities, and the interference from a radio sharing the same CPU.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours, including 30 minutes of setup.

Theory behind this lab (concept pages, in Romanian):
[Ce înseamnă timp real](https://github.com/automatica-cluj/rt-concepts/blob/main/00-what-is-real-time.md),
[Ce este un sistem de operare de timp real](https://github.com/automatica-cluj/rt-concepts/blob/main/01-what-is-an-rtos.md),
[Ceasuri și rezoluție](https://github.com/automatica-cluj/rt-concepts/blob/main/1-time/clocks-and-resolution.md),
[Latență și jitter](https://github.com/automatica-cluj/rt-concepts/blob/main/1-time/latency-and-jitter.md),
[Eliberare periodică](https://github.com/automatica-cluj/rt-concepts/blob/main/1-time/periodic-release.md).

---

## 1. Setup

Follow [../docs/setup.md](../docs/setup.md) up to and including step 3. When
`clocks` prints its first lines, you are ready.

Every sketch includes a copy of [../common/rt.h](../common/rt.h). The
functions `rt_now_us()`, `rt_cycles()`, `rt_busy_us()`, `rt_start_task()` and
`rt_verdict()` that you will see in the sketches come from there. You do not
need to read it now.

## 2. Clocks: `clocks`

Upload `clocks/clocks.ino`, open the serial monitor, press RST. Press `r` to
run it again.

The sketch prints five parts:

1. **Resolution.** The smallest step each clock can show. `millis()` moves in
   whole milliseconds, `micros()` and `esp_timer_get_time()` in microseconds,
   and the cycle counter in 1/160 µs.
2. **Cost.** How many CPU cycles one reading takes (the average of 1000
   readings, with the cost of measuring subtracted). A clock that costs more
   to read than the thing you time is useless for that job.
3. **Sleeping.** `delay(1)` and `vTaskDelay(1)` do not mean "1000 µs".
   FreeRTOS counts delays in ticks of 1 ms, and "one tick from now" can be
   only a few microseconds away if you call it just before a tick. That is
   why the minimum is so small.
4. **Busy waiting.** `delayMicroseconds()` watches the clock, so it stops on
   time. `rt_busy_us()` runs a fixed number of loop iterations, counted at
   start-up with interrupts off. With the tick interrupt now taking a little
   CPU every millisecond, it comes out slightly longer than asked. That is
   intended: every later lab uses `rt_busy_us()` as "computation time", and
   real computation is stretched by interrupts in the same way.
5. **Printing.** The cost of one `Serial.printf`, next to the cost of reading
   a clock. This is why the labs never print inside a timing loop: they record
   into an array and print afterwards.

Write down, for your report:

| clock                  | resolution | cost (cycles) |
|------------------------|------------|---------------|
| `millis()`             |            |               |
| `micros()`             |            |               |
| `esp_timer_get_time()` |            |               |
| cycle counter          |            |               |

and the minimum and maximum of `delay(1)`.

## 3. A periodic task: `periodic_jitter`

Upload `periodic_jitter/periodic_jitter.ino`. A task wants to run every
**T = 10 ms** and computes **C = 3 ms** each time. It records its wake-up
time in every period. After 300 periods it prints:

- **interval** between consecutive wake-ups: ideally exactly 10 000 µs
- **lateness**: wake-up time minus the ideal time `start + k*T`. A perfect
  task has lateness 0 in every period. **Jitter** is max − min lateness.
- **drift**: how much longer 299 periods took than 299 × T
- a histogram of lateness, and a `VERDICT:` line

Keys `a` to `e` select the variant. Write down the `VERDICT:` line of each
one.

### Experiment A — relative sleep (key `a`)

Press `a`. The task does its 3 ms of work, then sleeps with `vTaskDelay(10)`.

**Look at** the interval and the drift line.

**What it shows.** The interval is about T + C, not T: the 10 ms sleep starts
only after the 3 ms of work. Every period adds the same error, so the task
falls further behind with every period. That growing error is drift.

### Experiment B — absolute sleep (key `b`)

Press `b`. The task now uses `xTaskDelayUntil(&last, 10)`: "wake me at
`last + 10`", counted from the previous wake-up time, not from *now*.

**Look at** the interval, the drift and the jitter.

**What it shows.** The drift is gone: the work no longer adds to the period.
The jitter is very small, possibly 0. The task is woken by the tick interrupt,
and with nothing else running, getting from the tick to the task takes the
same time in every period. This run is the baseline for C, D and E.

### Experiment C — a higher-priority task in the way (key `c`)

Press `c`. A second task, the "hog", runs at priority 10, above our task
(priority 5). It is released every 9 ms and computes 2 ms. Because 9 ms is not
a multiple of 10 ms, it is sometimes running just when our task should wake
up. The hog has a deadline of its own, 4 ms after each release, and the
report prints a `hog:` line with its response times and missed deadlines.

**Look at** the lateness histogram next to the one from B, and the `hog:`
line.

**What it shows.** Our task is now late in some periods, by up to about 2 ms,
the hog's computation time: when the hog is running, our task has to wait
until it finishes. The hog itself meets its deadline, because nothing
interrupts it.

### Experiment D — raise our priority (key `d`)

Press `d`. The same hog, but our task now has priority 15, above the hog.

**Look at** the jitter, then the `hog:` line next to the one from C.

**What it shows.** Our task is on time again. The hog now waits whenever our
task is running, so its response time can grow to about 2 ms + 3 ms = 5 ms,
past its 4 ms deadline, and some of its jobs miss. Raising one priority did
not remove the interference. It moved it to the other task.

### Experiment E — the radio (key `e`)

Press `e`. No hog. Instead the Wi-Fi radio scans for networks. The Wi-Fi
driver runs as a task with priority 23 and in interrupts, both above any task
you create. Our task keeps priority 15.

**Look at** the jitter next to B and D, and the line
`late by more than 100 us`. Run `e` two or three times.

**What it shows.** Only a few of the 300 periods are late, and the average
stays close to 0, but the worst case is clearly worse than in B. For a
control loop the worst case is what counts. Raising our priority cannot help,
because the radio runs above every priority we can choose. Boards with two
cores, such as the ESP32 or ESP32-S3, solve this by running the radio on one
core and the control loop on the other.

### Optional — plot it

The plot script needs Python 3 with matplotlib. Install it once, in a
virtual environment inside the repository:

```
python3 -m venv .venv
.venv/bin/pip install matplotlib
```

On Windows: `py -m venv .venv`, then `.venv\Scripts\pip install matplotlib`,
and use `.venv\Scripts\python` instead of `.venv/bin/python` below.

After a run, press `p` and save the serial output to a file (copy it from the
monitor, or use `arduino-cli monitor ... | tee run.log`, see setup.md). Then:

```
.venv/bin/python tools/plot_serial.py run.log          # opens a window
.venv/bin/python tools/plot_serial.py run.log b.png    # writes a PNG instead
```

The left plot shows the interval between wake-ups in every period, the right
one a histogram of those intervals (log scale, so a single outlier is
visible). One late wake-up shows as a long interval followed by a short one.
Plotting B, C and E side by side shows the three cases at a glance.

### Optional — watch it on a logic analyzer (keys `a` to `d`)

This part needs a logic analyzer, for example a Saleae with the Logic 2
application. The sketch drives two pins, and they only change during a run,
in the 3 s after you press a key:

| Analyzer | Board pin | High while |
|---|---|---|
| channel 0 | `0` (GPIO0) | the periodic task computes (its C = 3 ms) |
| channel 1 | `1` (GPIO1) | a hog job runs, from its release until it is done |
| ground | `G` | |

On a Saleae, the channel wires are coloured like resistor bands: channel 0
is **black**, channel 1 brown. The grey wires are ground. Black is not
ground: clipping it to `G` leaves channel 0 empty. One grey wire on `G` is
enough, since all grounds are connected inside the analyzer. Take care that
the clip on `G` does not touch `5V` or `3V3` next to it.

In Logic 2, in the Device Settings panel:

- enable digital channels 0 and 1 only
- set the digital sample rate to 10 MS/s (0.1 µs steps, well below
  anything this sketch does). Higher rates also record nanosecond spikes
  from the clips, and one such spike is enough to fire the trigger
- choose the **Trigger** capture mode: rising edge on channel 0, 5 s of
  capture after the trigger
- if the trigger still fires before you press a key, turn on the glitch
  filter for channels 0 and 1 at about 1 µs. The real pulses are
  milliseconds long, so the filter removes only spikes

Press Start. Logic 2 waits for the trigger. Then press the key in the
serial monitor: the capture starts at the first period of the run and stops
by itself. Press Start again before the next key. Hovering over a pulse
shows its width, and a pair of Timing Markers on two edges shows the time
between them.

**A (key `a`).** Only channel 0 moves.

**Look at** the width of a pulse, the time from one rising edge to the next,
and the time from the first rising edge of the run to the last.

**What it shows.** The pulse is C, 3 ms. The rising edges are T + C apart:
3 ms high, then the full 10 ms sleep low. The whole run takes longer than
299 × 10 ms by the drift the report printed. The drift is measured here on a
clock the board does not control.

**B (key `b`).** Only channel 0 moves.

**Look at** the time between rising edges, then zoom in on a few edges.

**What it shows.** The rising edges are 10 ms apart, the low part shrinks to
T − C, and the edges move by far less than the 1 ms tick. This is the
baseline.

**C (key `c`).** Both channels move: 2 ms pulses every 9 ms on channel 1,
3 ms pulses every 10 ms on channel 0. Because 9 ms and 10 ms differ, the two
slide past each other.

**Look at** a place where a channel 1 pulse is high at the moment channel 0
should rise.

**What it shows.** There, channel 0 rises only when channel 1 falls: our
task was ready, but the hog had the CPU. The distance from the 10 ms grid to
that edge is the lateness in the report, up to one hog job.

**D (key `d`).** Both channels move, as in C.

**Look at** the same kind of overlap.

**What it shows.** Channel 0 now rises on the grid every time. Instead, the
channel 1 pulse is 3 ms longer wherever the two overlap: it stays high while
channel 0 is high, because the hog job was preempted and finished later. The
interference moved from our task to the hog, which is what the `hog:` line in
D reports as longer response times and missed deadlines.

Key `e` drives only channel 0. The Wi-Fi activity that delays it is not on a
pin, so the analyzer shows the few late edges but not their cause.

**Cross-check.** The sketch measures itself with the board's own clock, and
the analyzer measures the same events from outside. Export the channel 0
edges (File, Export Data, CSV), compute
lateness = edge − (first edge + k × 10 ms) for each period, and compare
max − min with the jitter the sketch printed. The two agree closely: the pin
goes high a few instructions after the sketch takes its timestamp, and
`rt_now_us()` counts whole microseconds. The analyzer also shows something
the sketch cannot. Over the 3 s of a B run, the time from the first to the
last rising edge differs slightly from 299 × 10 ms, because the board's
crystal is not exactly its nominal frequency. The sketch cannot see this,
since it measures time with that same crystal. Strictly, the analyzer shows
the difference between two crystals, the board's and its own.

## Rules for these labs

- Every experiment ends with a `VERDICT:` line. Quote it in your report.
- If the board stops responding, see the recovery steps in setup.md. Nothing
  you do in these labs can damage it.
