# Lab 2 — Tasks and fixed-priority scheduling

FreeRTOS follows one rule: the highest-priority task that is ready runs. In
this lab you watch that rule at work on a single CPU: preemption, equal
priorities sharing the CPU by time slicing, a periodic task cutting into a
long one. Then you break it on purpose with a task that never lets go of the
CPU, and let the watchdog clean up.

Time: about 2 hours.

Theory behind this lab (concept pages, in Romanian):
[Taskuri și stări](https://github.com/automatica-cluj/rt-concepts/blob/main/2-scheduling/tasks-and-states.md),
[Preempțiune cu priorități fixe](https://github.com/automatica-cluj/rt-concepts/blob/main/2-scheduling/fixed-priority-preemption.md),
[Înfometare și watchdog](https://github.com/automatica-cluj/rt-concepts/blob/main/2-scheduling/starvation-and-watchdogs.md).

Read [../docs/freertos_concepts.md](../docs/freertos_concepts.md) first,
sections "Tasks and priorities", "Task states" and "The tick".

---

## 1. How `scheduling` works

Upload `scheduling/scheduling.ino`.

- Every **worker task** burns CPU time with `rt_tl_busy()`. While it does, it
  writes its letter into a **timeline**: one character per slice of time
  (5 ms or 1 ms).
- `loop()` is the control code. While it releases workers it raises itself to
  priority 10, above all of them, so a worker cannot start before its release
  time. It creates each worker at a fixed moment, then blocks until all are
  done, then prints.
- After the strip, a table gives each worker's release, start and finish time
  and its **response time** (finish − release). The verdict is the order in
  which the workers finished.

The ESP32-C3 has one core, so every worker competes for the same CPU. In the
Linux labs you had to pin threads to one CPU to get this; here it is simply
the hardware.

## 2. Experiments

### Experiment A — preemption (key `a`)

Press `a`. L (priority 2) is released at 0 ms, M (3) at 40 ms, H (4) at
80 ms, and each computes 100 ms.

Before you look: draw the timeline you expect on paper, and predict each
response time. Then compare. The measured times come out a few percent longer
than your prediction: the tick interrupt takes a little CPU time every
millisecond, and marking the strip costs a clock reading every 100 µs. The
response time of L is not 100 ms. Why not, and what does it consist of?

### Experiment B — equal priorities (key `b`)

Press `b`. A, B and C all have priority 3, are released together and compute
40 ms each. The strip has 1 ms columns.

FreeRTOS switches between ready tasks of *equal* priority at every tick. How
long is each turn? Why do all three finish at nearly the same time, much later
than 40 ms? Compare with running them one after the other: which is better for
the *average* response time, and which for the first one to finish?

### Experiment C — a periodic task and a long one (key `c`)

Press `c`. H (priority 4) computes 4 ms every 20 ms, eight times. L
(priority 2) computes 120 ms in one piece.

Find H's jobs in the strip. What is H's response time for each job? (The
table gives no response time for H: it has eight, one per job, and the strip
shows them.) What is
L's response time, and how can you compute it from H's parameters? (This is
the idea behind response-time analysis in Lab 3.)

### Experiment D — who else is running? (key `d`)

Press `d`. Same as A, plus a snapshot of every task on the chip taken at
60 ms, printed with `vTaskList()`.

Find `loopTask`, `IDLE`, `esp_timer`, `Tmr Svc` and the three workers. For
each, note the state (X running, R ready, B blocked, S suspended) and the
priority. Why is `loopTask` shown as running? Why is L ready and not blocked?
The "stack" column is the smallest amount of stack that has ever been free, in
bytes. Which task came closest to running out?

The snapshot itself took time: `loopTask` ran at priority 10 while it
collected it. The strip cannot show that, because a column is 5 ms wide.
Compare the table with the one from A instead. Which times moved, which did
not, and by how much? That is how long the snapshot took.

## 3. Starvation and the watchdog: `starvation_wdt`

Upload `starvation_wdt/starvation_wdt.ino`. `loop()` runs at priority 1,
blinks the LED and measures the longest gap between two of its own runs. The
**task watchdog** is set to 3 s and watches `loop()`: if `loop()` does not
come round for 3 s, the watchdog resets the chip.

Keys start a **hog** task at priority 5 that computes without blocking.

### Experiment E — starved, but not dead (keys `a`, `c`, `b`, `d`)

1. Press `a`. The hog computes for 1 s. Watch the LED. When the hog ends, the
   sketch prints the longest gap between two `loop()` runs.
2. Press `c`. The hog computes for 6 s, but sleeps 1 ms after every 10 ms of
   work. What is the longest gap now? Why does a 1 ms pause make such a
   difference to a task at priority 1?
3. Press `b`. The hog computes for 6 s without pausing. After about 3 s the
   watchdog fires. It prints which task failed to check in, then a register
   dump and a long block of stack memory (you can ignore both), and resets
   the chip. The sketch starts again and prints the reset reason and the
   variant that was running. Press `i` to print that report again.
4. Press `d`. The hog now registers itself with the watchdog and feeds it
   diligently while it computes. Does that save the board? Why not? What does
   the watchdog actually watch?

## Notes

- The watchdog reset in experiment E is expected and harmless.
- If the serial monitor does not reconnect after the reset, close and reopen
  it, or press RST.
