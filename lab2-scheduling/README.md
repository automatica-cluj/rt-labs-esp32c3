# Lab 2 — Tasks and fixed-priority scheduling

FreeRTOS follows one rule: the highest-priority task that is ready runs. In
this lab you watch that rule at work on a single CPU: preemption, equal
priorities sharing the CPU by time slicing, a periodic task cutting into a
long one. Then you see what happens when a task never lets go of the CPU,
and how the watchdog cleans up.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours.

Theory behind this lab (concept pages, in Romanian):
[Taskuri și stări](https://github.com/automatica-cluj/rt-concepts/blob/main/2-scheduling/tasks-and-states.md),
[Preempțiune cu priorități fixe](https://github.com/automatica-cluj/rt-concepts/blob/main/2-scheduling/fixed-priority-preemption.md),
[Înfometare și watchdog](https://github.com/automatica-cluj/rt-concepts/blob/main/2-scheduling/starvation-and-watchdogs.md).

The sections "Tasks and priorities", "Task states" and "The tick" in
[../docs/freertos_concepts.md](../docs/freertos_concepts.md) introduce the
terms used below.

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

All measured times come out a few percent longer than the plain arithmetic.
The tick interrupt takes a little CPU time every millisecond, and marking the
strip costs a clock reading every 100 µs.

## 2. Experiments

### Experiment A — preemption (key `a`)

Press `a`. L (priority 2) is released at 0 ms, M (3) at 40 ms, H (4) at
80 ms, and each computes 100 ms.

**Look at** the strip and the response times in the table.

**What it shows.** Each new task with a higher priority takes the CPU at once:

```
  0– 40 ms  L runs
 40– 80 ms  M arrives and preempts L
 80–180 ms  H arrives and preempts M, runs to the end
180–240 ms  M finishes its remaining 60 ms
240–300 ms  L finishes its remaining 60 ms
```

H's response time is about its own 100 ms. L's is about 300 ms: its own
100 ms plus the 200 ms that M and H ran in the middle of it. A task's response
time is its own computation plus the time taken by higher-priority tasks.

### Experiment B — equal priorities (key `b`)

Press `b`. A, B and C all have priority 3, are released together and compute
40 ms each. The strip has 1 ms columns.

**Look at** the strip, and the finish times in the table.

**What it shows.** The letters alternate every column: FreeRTOS switches
between ready tasks of equal priority at every tick, so each turn is 1 ms.
All three advance together and finish close to 120 ms, although each needs
only 40 ms. Run one after the other, they would finish at 40, 80 and 120 ms:
a better average, but not fair to the last one. Time slicing shares the CPU
evenly, at the cost of making everyone finish late.

### Experiment C — a periodic task and a long one (key `c`)

Press `c`. H (priority 4) computes 4 ms every 20 ms, eight times. L
(priority 2) computes 120 ms in one piece. Both start at 0 ms.

**Look at** H's jobs in the strip (the table gives no response time for H,
because it has eight jobs), and L's response time in the table.

**What it shows.** Every H job starts at once and takes about 4 ms: H always
has the CPU when it needs it. L runs in the gaps. Its response time is its own
120 ms plus the eight H jobs of 4 ms that fall inside it, about 152 ms. Adding
up the interference of the higher-priority tasks this way is the idea behind
response-time analysis in Lab 3.

### Experiment D — who else is running? (key `d`)

Press `d`. Same as A, plus a snapshot of every task on the chip taken at
60 ms, printed with `vTaskList()`.

**Look at** the snapshot table: find `loopTask`, `IDLE`, `esp_timer`,
`Tmr Svc` and the workers, with their state (X running, R ready, B blocked,
S suspended) and priority.

**What it shows.** There are more tasks than the ones you created: the idle
task, the timer tasks, and `loopTask`, which runs `loop()`. `loopTask` is
shown as running because it is the task that takes the snapshot. At 60 ms M
is ready: it wants the CPU and is waiting only for `loopTask` to finish the
snapshot. L is ready too, not blocked: it is not waiting for anything, it
has only been preempted. H does not appear yet, because it is released at
80 ms. The "stack" column is the least free stack each task has had, in bytes.

The snapshot took time, too little to see in the 5 ms columns. Compare the
response times with the ones from A: L and M, which were ready at 60 ms,
grew by the time the snapshot took, and H did not.

## 3. Starvation and the watchdog: `starvation_wdt`

Upload `starvation_wdt/starvation_wdt.ino`. `loop()` runs at priority 1,
blinks the LED and measures the longest gap between two of its own runs. The
**task watchdog** is set to 3 s and watches `loop()`: if `loop()` does not
come round for 3 s, the watchdog resets the chip.

Keys start a **hog** task at priority 5 that computes without blocking.

### Experiment E — starved, but not dead (keys `a`, `c`, `b`, `d`)

1. Press `a`. The hog computes for 1 s. The LED stops blinking for that
   second, and the sketch prints a longest gap of about 1 s. `loop()` was
   starved, but for less than 3 s, so the watchdog stays quiet.
2. Press `c`. The hog computes for 6 s, but sleeps 1 ms after every 10 ms of
   work. The LED keeps blinking and the longest gap is about 10 ms. Each
   1 ms pause is enough for `loop()` to run, because while the hog sleeps,
   `loop()` is the highest-priority task that is ready.
3. Press `b`. The hog computes for 6 s without pausing. After about 3 s the
   watchdog fires. It prints which task failed to check in, then a register
   dump and a long block of stack memory (you can ignore both), and resets
   the chip. The sketch starts again and prints the reset reason and the
   variant that was running. Press `i` to print that report again.
4. Press `d`. The hog now registers itself with the watchdog and feeds it
   while it computes. The board still resets. The watchdog watches every
   task registered with it, and each one must check in on its own. The hog
   feeding it proves only that the hog is alive. `loop()` is still starved
   and cannot check in.

## Notes

- The watchdog resets in experiment E are expected and harmless.
- If the serial monitor does not reconnect after the reset, close and reopen
  it, or press RST.
