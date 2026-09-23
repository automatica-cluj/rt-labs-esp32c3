# Lab 3 — Periodic tasks, WCET and Rate Monotonic scheduling

Most real-time software is a set of periodic tasks: read a sensor every 10 ms,
run a controller every 15 ms, log every 35 ms. To know in advance that every
deadline will be met you need two things: the worst-case execution time
(WCET) of each task, and a schedulability test. In this lab you see how the
WCET is measured, and how the tests compare with what the board actually
does.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours.

Theory behind this lab (concept pages, in Romanian):
[Modelul de task](https://github.com/automatica-cluj/rt-concepts/blob/main/3-analysis/task-model.md),
[WCET](https://github.com/automatica-cluj/rt-concepts/blob/main/3-analysis/wcet.md),
[Rate Monotonic](https://github.com/automatica-cluj/rt-concepts/blob/main/3-analysis/rate-monotonic.md),
[Analiza timpului de răspuns](https://github.com/automatica-cluj/rt-concepts/blob/main/3-analysis/response-time-analysis.md),
[Supraîncărcare](https://github.com/automatica-cluj/rt-concepts/blob/main/3-analysis/overload.md).

---

## 1. Terms

- **T** — period: a new job of the task is **released** every T
- **C** — computation time of one job; the worst case is the **WCET**
- **D** — relative deadline; here D = T: a job must finish before the next
  release
- **R** — response time: release to completion. The job **misses** if R > D.
- **U** — utilisation: the sum of C/T over all tasks. U > 1 can never work.
- **Rate Monotonic (RM)**: fixed priorities, the shorter the period, the
  higher the priority

## 2. Measuring WCET: `wcet_measure`

Upload `wcet_measure/wcet_measure.ino`. The "job" is an insertion sort of 64
numbers followed by a moving average. It is timed with the CPU cycle counter.

Experiments A, B and D sort the **same** input in every run: 64 random
numbers, made once at start-up (with a fixed seed, so every board gets the
same ones). The input therefore cannot be what makes one run slower than
another. Experiment C is about the input.

Right after reset the sketch prints the time of the **first** run and the
**second** run of the job, on that same input. The first is slower: its code
is not in the cache yet. Experiment D comes back to this.

### Experiment A — 1000 runs, interrupts on (key `a`)

Press `a`.

**Look at** min, avg and max, and the histogram. It shows how many cycles
each run took above the fastest run (160 cycles = 1 µs).

**What it shows.** Almost all runs take the same time. A few are clearly
slower, and they make the max. Those are the runs during which an interrupt
arrived, mostly the 1 ms tick: the CPU left the job, handled the interrupt
and came back, and the cycle counter kept counting. The shorter the job
compared with 1 ms, the fewer runs are hit.

### Experiment B — interrupts off (key `b`)

Press `b`. Each run is now measured with interrupts off
(`portENTER_CRITICAL`).

**Look at** the max next to the one from A.

**What it shows.** The slow runs are gone and max is close to avg. This is
the time of the job itself. A's max was the job plus an interrupt. A
schedulability analysis keeps the two apart: the job's own worst case is its
C, and the interrupts are counted separately as interference.

### Experiment C — input matters (key `c`)

Press `c`. The same job on already-sorted, random and reverse-sorted input.
Here "random" is a new random input in every run.

**Look at** the three rows.

**What it shows.** Sorted input is the fastest and reverse-sorted input the
slowest. Insertion sort moves every element past all the ones before it when
the input is reversed, the most work it can do. The random row spreads out
between the two, but random inputs almost never happen to be fully reversed,
so random testing would not find the worst case. The worst input has to be
worked out from how the code behaves.

### Experiment D — a cold cache (key `d`)

Press `d`. Before every run, the sketch reads a 64 KB table from flash. That
pushes the job's code out of the 16 KB cache, so it has to come from flash
again. (The cache only holds what lives in flash: code and constants. The
arrays the job sorts are in RAM and are not cached.)

**Look at** the times next to B and to the "first run" printed at reset.

**What it shows.** Every run is now slower than in B, and similar to the first
run after reset: each one starts with a cold cache. So the WCET of a piece of
code is not a property of the code alone. In A–D it changed with interrupts,
with the input, and with the state of the cache.

## 3. A task set: `rm_taskset`

Upload `rm_taskset/rm_taskset.ino`. Three periodic tasks, `sensor`, `control`
and `logger`, each computing C ms (with `rt_busy_us`) every T ms. All three are
released together at t0 (the *critical instant*, the worst case for fixed
priorities) and run for 2.1 s. For each task set the sketch prints:

- U and the **Liu & Layland bound** n(2^(1/n) − 1), 0.780 for three tasks.
  If U is at or below the bound, RM meets every deadline. If U is above it,
  the test cannot tell.
- The worst-case response time from **response-time analysis (RTA)**:
  R = C_i + Σ over higher-priority j of ⌈R / T_j⌉ · C_j, iterated until it
  stops changing. In words: a task's own computation, plus every job of a
  higher-priority task released while it is still waiting or running. If
  R ≤ D, the task always meets its deadline. RTA is exact for this model.
- What the board measured: max and average R, jobs, misses and skipped
  releases.

The task sets are tables at the top of the sketch:

| Key | Set (T, C) in ms | U | Priorities |
|---|---|---|---|
| `a` | sensor (10, 2), control (20, 4), logger (40, 8) | 0.60 | RM |
| `b` | sensor (10, 3), control (15, 4), logger (35, 10) | 0.85 | RM |
| `c` | sensor (10, 3), control (15, 5), logger (35, 14) | 1.03 | RM, late jobs skipped |
| `d` | same as b | 0.85 | reversed: logger highest |
| `e` | same as c | 1.03 | RM, late jobs run late |

### Experiment E — below the bound, above the bound (keys `a`, `b`)

Press `a`, then `b`.

**Look at** the line with U and the bound, the RTA column, and the measured
R max and misses.

**What it shows.** Set a is below the bound, so it is schedulable, and there
are no misses. Set b is above the bound, so the Liu & Layland test cannot
tell. RTA gives every task an R below its deadline, and indeed the board
misses nothing. The bound is sufficient, not necessary: passing it guarantees
the deadlines, and failing it does not mean a miss. The measured R max comes
out a little above RTA, because RTA does not count the tick interrupt and
the task switches.

### Experiment F — overload (key `c`)

Press `c`. U > 1, so something must miss.

**Look at** the `miss` column.

**What it shows.** Only the logger misses. The sensor and control are not
affected at all. With fixed priorities, an overload hurts the
lowest-priority tasks first, and the high-priority ones keep their
deadlines.

### Experiment G — the wrong priorities (key `d`)

Press `d`: set b again, U = 0.85, but the logger (the longest period) now has
the highest priority.

**Look at** the `miss` column, next to set b in experiment E.

**What it shows.** The same task set that met every deadline now misses. The
sensor, the task with the shortest period, misses: its 10 ms period cannot
fit behind the logger's 10 ms and the control's 4 ms. Priorities chosen by
"importance" instead of by period can break a task set that is schedulable
with RM.

### Experiment H — overrun policy (key `e`)

Press `e`: the overload of c, but a job that is late is still run, as soon as
possible (catch-up), instead of being skipped. `xTaskDelayUntil` does exactly
this by default when the wake-up time has already passed.

**Look at** the logger's R max and skipped count, next to c.

**What it shows.** No job is skipped, but the logger's response time is much
larger than in c. With U > 1 there is never time to catch up, so the backlog
and the response time keep growing the longer it runs. Skipping gives up some
jobs to keep the rest on time. Catching up keeps every job, but later and
later. For a logger, keeping every entry may be worth the delay. For a motor
controller, a late output is useless, so skipping is better.
