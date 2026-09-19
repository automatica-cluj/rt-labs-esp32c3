# Lab 2 — Tasks and fixed-priority scheduling

FreeRTOS follows one rule: the highest-priority task that is ready runs. In
this lab you watch that rule at work on a single CPU: preemption, equal
priorities sharing the CPU by time slicing, a periodic task cutting into a
long one. Then you break it on purpose with a task that never lets go of the
CPU, and let the watchdog clean up.

Time: about 2 hours.

Read [../docs/freertos_concepts.md](../docs/freertos_concepts.md) first,
sections "Tasks and priorities", "Task states" and "The tick".

---

## 1. How `scheduling` works

Upload `scheduling/scheduling.ino`.

- Every **worker task** burns CPU time with `tl_burn()`. While it does, it
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

### Experiment A — preemption

Press `a`. L (priority 2) is released at 0 ms, M (3) at 40 ms, H (4) at
80 ms, and each computes 100 ms.

Before you look: draw the timeline you expect on paper, and predict each
response time. Then compare. The response time of L is not 100 ms. Why not,
and what does it consist of?

### Experiment B — equal priorities

Press `b`. A, B and C all have priority 3, are released together and compute
40 ms each. The strip has 1 ms columns.

FreeRTOS switches between ready tasks of *equal* priority at every tick. How
long is each turn? Why do all three finish at nearly the same time, much later
than 40 ms? Compare with running them one after the other: which is better for
the *average* response time, and which for the first one to finish?

### Experiment C — a periodic task and a long one

Press `c`. H (priority 4) computes 4 ms every 20 ms, eight times. L
(priority 2) computes 120 ms in one piece.

Find H's jobs in the strip. What is H's response time for each job? What is
L's response time, and how can you compute it from H's parameters? (This is
the idea behind response-time analysis in Lab 3.)

### Experiment D — who else is running?

Press `d`. Same as A, plus a snapshot of every task on the chip taken at
60 ms, printed with `vTaskList()`.

Find `loopTask`, `IDLE`, `esp_timer`, `Tmr Svc` and the three workers. For
each, note the state (X running, R ready, B blocked, S suspended) and the
priority. Why is `loopTask` shown as running? Why is L ready and not blocked?
The "stack" column is the smallest amount of stack that has ever been free, in
bytes. Which task came closest to running out?

The snapshot itself took time. Look for the gap it left in the strip around
60 ms.

## 3. Starvation and the watchdog: `starvation_wdt`

Upload `starvation_wdt/starvation_wdt.ino`. `loop()` runs at priority 1,
blinks the LED and measures the longest gap between two of its own runs. The
**task watchdog** is set to 3 s and watches `loop()`: if `loop()` does not
come round for 3 s, the watchdog resets the chip.

Keys start a **hog** task at priority 5 that computes without blocking.

### Experiment E — starved, but not dead

1. Press `a`. The hog computes for 1 s. Watch the LED. When the hog ends, the
   sketch prints the longest gap between two `loop()` runs.
2. Press `c`. The hog computes for 6 s, but sleeps 1 ms after every 10 ms of
   work. What is the longest gap now? Why does a 1 ms pause make such a
   difference to a task at priority 1?
3. Press `b`. The hog computes for 6 s without pausing. After about 3 s the
   board resets. The USB port disappears and comes back. Press `i` in the
   serial monitor (reconnect it if needed) to see the reset reason and the
   variant that was running.
4. Press `d`. The hog now registers itself with the watchdog and feeds it
   diligently while it computes. Does that save the board? Why not? What does
   the watchdog actually watch?

## 4. Questions

1. In experiment A, write down the response time of each task as a sum of its
   own computation and the time taken by others.
2. The control code runs at priority 10 while it releases workers. What would
   the timeline of A look like if it ran at priority 1 instead? Try it: change
   `kControlPrio`.
3. Time slicing is on in experiment B. Name one situation where you would
   rather not have it.
4. In experiment C, what is the longest time L can be delayed by H in any
   window of 20 ms? What if H's computation grows to 20 ms?
5. On Linux, `SCHED_FIFO` tasks are held back by RT throttling after 950 ms
   of every second. What does the FreeRTOS setup here do instead, and which
   approach is better for (a) a desktop, (b) a pacemaker?
6. Experiment E step 4: why is it a bad idea to feed a watchdog from inside a
   loop that can run forever? Where should the feeding happen?

## Notes

- The watchdog reset in experiment E is expected and harmless.
- If the serial monitor does not reconnect after the reset, close and reopen
  it, or press RST.
