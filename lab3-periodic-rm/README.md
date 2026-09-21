# Lab 3 — Periodic tasks, WCET and Rate Monotonic scheduling

Most real-time software is a set of periodic tasks: read a sensor every 10 ms,
run a controller every 15 ms, log every 35 ms. To know in advance that every
deadline will be met you need two things: the worst-case execution time
(WCET) of each task, and a schedulability test. In this lab you measure the
first and check the second against what the board actually does.

Time: about 2 hours.

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

Right after reset the sketch prints the time of the **first** run and the
**second** run of the job. Note both.

### Experiment A — 1000 runs, interrupts on

Press `a`. Note min, avg and max, and the histogram. The max is noticeably
larger than the average, and only a few runs are responsible. What could
happen during those few runs that does not happen during the others?

### Experiment B — interrupts off

Press `b`. Each run is now measured with interrupts off
(`portENTER_CRITICAL`). Compare the max with A. What did A's max really
measure? Which of the two numbers belongs in a schedulability analysis, and
why is the answer "neither, on its own"?

### Experiment C — input matters

Press `c`. The same job on already-sorted, random and reverse-sorted input.
Which input is the worst case for insertion sort, and why? Would random
testing have found it?

### Experiment D — a cold cache

Press `d`. Before every run, the sketch reads a 64 KB table from flash. That
pushes the job's code and data out of the 16 KB cache, so they have to come
from flash again. Compare with B and with the "first run" printed at reset.

The WCET of a piece of code is not a property of the code alone. List
everything you have seen in A–D that changes it.

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
  stops changing. If R ≤ D, the task always meets its deadline. RTA is exact
  for this model.
- What the board measured: max and average R, jobs, misses and skipped
  releases.

The task sets are tables at the top of the sketch. Before each run, compute U
and the RTA values by hand for at least one task, then check your numbers
against the sketch.

| Key | Set (T, C) in ms | U | Priorities |
|---|---|---|---|
| `a` | sensor (10, 2), control (20, 4), logger (40, 8) | 0.60 | RM |
| `b` | sensor (10, 3), control (15, 4), logger (35, 10) | 0.85 | RM |
| `c` | sensor (10, 3), control (15, 5), logger (35, 14) | 1.03 | RM, late jobs skipped |
| `d` | same as b | 0.85 | reversed: logger highest |
| `e` | same as c | 1.03 | RM, late jobs run late |

### Experiment E — below the bound, above the bound

Press `a`, then `b`. Set b is above the Liu & Layland bound. Is it
schedulable? What does RTA say, and what did the board do? What does "the
bound is sufficient, not necessary" mean in terms of this result?

Compare the measured R max with RTA for each task. The measurement is a little
higher. Where does the difference come from?

### Experiment F — overload

Press `c`. U > 1, so something must miss. Which task? Why is it that one and
not the others? This is one of the most useful properties of fixed-priority
scheduling. Put it in one sentence.

### Experiment G — the wrong priorities

Press `d`: set b again, U = 0.85, but the logger (the longest period) now has
the highest priority. Which task misses now, and why? What does this tell you
about choosing priorities by "importance"?

### Experiment H — overrun policy

Press `e`: the overload of c, but a job that is late is still run, as soon as
possible (catch-up), instead of being skipped. `xTaskDelayUntil` does exactly
this by default when the wake-up time has already passed.

Compare the logger's max response time with c. What happens to it over a
longer run? For a logger, which policy is better? For a motor controller?
