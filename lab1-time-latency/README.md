# Lab 1 — Time, latency and jitter

Get the board running, find out what the clocks on it can measure, and see how
regular a periodic task really is: relative against absolute sleeps, the
effect of priorities, and the interference you get from a radio sharing the
same CPU.

Time: about 2 hours, of which the first 30 minutes are setup.

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

Read [../common/rt.h](../common/rt.h) once. Every sketch includes a copy
of it: `rt_now_us()`, `rt_cycles()`, `rt_busy_us()`, `struct rt_stats`, `rt_start_task()` and
`rt_verdict()` come from there.

## 2. Clocks: `clocks`

Upload `clocks/clocks.ino`, open the serial monitor, press RST. Press `r` to
run it again.

The sketch prints five parts:

1. **Resolution.** The smallest step each clock can show. `millis()` moves in
   whole milliseconds, `micros()` and `esp_timer_get_time()` in microseconds,
   and the cycle counter in 1/160 µs.
2. **Cost.** How many CPU cycles one reading takes. The sketch reads the cycle
   counter just before and just after each reading, and subtracts what the
   same pair costs with nothing in between, so the measuring itself is not
   counted. The figure is the average of 1000 readings. A clock that costs
   more to read than the thing you time is useless for that job.
3. **Sleeping.** `delay(1)` and `vTaskDelay(1)` do not mean "1000 µs". Look at
   the minimum. FreeRTOS counts delays in ticks of 1 ms, and "one tick from
   now" can be a few microseconds away if you call it just before a tick.
4. **Busy waiting.** `delayMicroseconds()` and `rt_busy_us()` both keep the CPU.
   Compare their accuracy. `delayMicroseconds()` watches the clock, so it
   stops on time. `rt_busy_us()` runs a fixed number of loop iterations,
   counted at start-up with interrupts off. Now the tick interrupt comes
   every millisecond and takes a little of the CPU each time, so the same
   iterations take slightly longer than asked. That is intended: `rt_busy_us()`
   is how every later lab models "computation time", and real computation is
   stretched by interrupts and preemption in the same way.
5. **Printing.** The cost of one `Serial.printf`. Compare it with part 2. This
   is why the labs never print inside a timing loop: record into an array,
   print afterwards.

Write down, for your report: the resolution and cost of each clock, and the
minimum and maximum of `delay(1)`.

## 3. A periodic task: `periodic_jitter`

Upload `periodic_jitter/periodic_jitter.ino`. A task with priority 5 wants to
run every **T = 10 ms** and computes **C = 3 ms** each time. It records its
wake-up time in every period. After 300 periods it prints:

- **interval** between consecutive wake-ups: ideally exactly 10 000 µs
- **lateness**: wake-up time minus the ideal time `start + k*T`. A perfect
  task has lateness 0 in every period. **Jitter** is max − min lateness.
- **drift**: how much longer 299 periods took than 299 × T
- a histogram of lateness, and a `VERDICT:` line

Keys `a` to `e` select the variant, and `p` prints the last run as CSV.

### Experiment A — relative sleep (key `a`)

Press `a`. The task sleeps with `vTaskDelay(10)` *after* its work.

Look at the interval and the drift. Explain the interval from T and C. What
would a clock that "ticks" with this task show after one hour?

### Experiment B — absolute sleep (key `b`)

Press `b`. The task uses `xTaskDelayUntil(&last, 10)`: "wake me at
`last + 10`", measured from the previous *release*, not from *now*.

The drift should be gone, and the jitter very small, possibly 0. The task is
woken by the tick interrupt. With nothing else running, the way from the tick
to the task takes the same time in every period, and lateness is measured
against the earliest wake-up, so a constant delay does not show up as jitter.
This run is the baseline for C, D and E. Before you run them: what could make
that way longer in some periods and not in others?

### Experiment C — a higher-priority task in the way (key `c`)

Press `c`. A "hog" task at priority 10 is released every 9 ms and computes
2 ms each time. Its period is not a multiple of 10 ms, so sometimes it is
running exactly when our task should wake up. The hog has a deadline of its
own: each job must be done 4 ms after its release. It measures its response
time (release to finish), and the report prints a `hog:` line with the
response times and the number of missed deadlines.

Compare the lateness histogram with B. What is the largest lateness you would
expect from the hog's parameters? Does the measurement agree?

### Experiment D — raise our priority (key `d`)

Press `d`. Same hog, but our task now has priority 15. What changed for our
task? Now compare the `hog:` line with the one from C. What is the longest
response time the hog can now have, given our task's T and C? Does the hog miss its
deadline, and in how many jobs? Raising one task's priority did not remove
the interference. Where did it go?

### Experiment E — the radio (key `e`)

Press `e`. No hog. Instead the Wi-Fi radio scans for networks. The Wi-Fi driver
runs as a task with priority 23 and in interrupts, both above any task you
create.

Compare the jitter with B and D. The effect is rare: only a few of the 300
periods are late, and the average lateness stays close to 0. Look at the
line `late by more than 100 us`. Run `e` three times and write down the
jitter and that count each time. Which one matters for a control loop, the
average or the worst case?

Our task has priority 15, yet it is delayed.
Why can raising its priority not fix this? What would you do in a real device
that needs both Wi-Fi and a tight control loop? (The ESP32 and ESP32-S3 have
two cores. How would that help?)

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

The window may open behind the terminal, and the script ends when you close
it. The left plot shows the interval between wake-ups for every period, the
right one a histogram of those intervals (the count is on a log scale, so a
single outlier is visible). Note that it plots intervals, not lateness: one
late wake-up shows as a long interval followed by a short one. If the file
holds several `csv:` blocks (you pressed `p` more than once), each one is
drawn in its own colour on the same axes.

Do this for B, C and E, each in its own file (`b.png`, `c.png`, `e.png`), and
put the three plots side by side.

## Rules for these labs

- Every experiment ends with a `VERDICT:` line. Quote it in your report.
- If the board stops responding, see the recovery steps in setup.md. Nothing
  you do in these labs can damage it.
