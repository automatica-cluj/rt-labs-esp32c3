# Lab 5 — Shared resources: races, priority inversion, deadlock

As soon as two tasks share data, three things can go wrong. Updates get lost
(a race). A high-priority task ends up waiting for a low-priority one, and
then for a medium one that has nothing to do with it (priority inversion).
Or two tasks wait for each other forever (deadlock). You make each of them
happen on the board, then fix it.

Time: about 2 hours.

Theory behind this lab (concept pages, in Romanian):
[Condiții de cursă](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/race-conditions.md),
[Excludere mutuală](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/mutual-exclusion.md),
[Inversiunea de prioritate](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/priority-inversion.md),
[Deadlock](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/deadlock.md).

Read "Sharing data" in
[../docs/freertos_concepts.md](../docs/freertos_concepts.md) first.

---

## 1. Lost updates: `race`

Upload `race/race.ino`. Two tasks each add 1 to a shared counter a million
times. The right answer is 2 000 000.

`g_counter = g_counter + 1` compiles to three instructions: load, add, store.
If the scheduler switches to the other task between the load and the store,
the first task later stores a stale value, and everything the other task
added in the meantime is gone.

### Experiment A — the race

Press `a`: both tasks at priority 3, no protection. Run it three times. How
many updates are lost, and is the number the same each time? Why is a single
lost update so expensive here? (Hint: how long does a task run before the
tick switches to the other one?)

### Experiment B — the race that hides

Press `b`: the same code, but priorities 3 and 4. Nothing is lost. Explain
why. Is the code correct now? What would break it again?

### Experiment C — three fixes

Press `c` (mutex), `d` (critical section) and `e` (`std::atomic`). All three
give 2 000 000. Compare how long each takes. The C3 has no atomic
instructions, so `fetch_add` briefly disables interrupts. Which fix would you
use for a counter that an ISR also increments, and why can the mutex not be
used there?

## 2. Priority inversion: `inversion`

Upload `inversion/inversion.ino`. Three tasks on one CPU, one lock:

```
   0 ms  L (priority 2) takes the lock, computes 40 ms inside it
  10 ms  H (priority 5) wants the lock, has to wait for L
  20 ms  M (priority 3) computes 150 ms, does not touch the lock
```

The sketch prints a timeline (5 ms per column), when H asked for the lock and
when it got it, the highest priority L ran at while holding the lock, and a
verdict: **BOUNDED** if H waited no longer than L's critical section, and
**UNBOUNDED** otherwise.

### Experiment D — binary semaphore

Press `a`. The lock is a binary semaphore. Read the timeline: who runs from
20 ms on? How long was H blocked? H has the highest priority on the chip, yet
it waited for M, which has nothing to do with the lock.

### Experiment E — mutex with priority inheritance

Press `b`. The lock is now a FreeRTOS mutex. Read the timeline again. What
priority did L run at while holding the lock? When did H get the lock? Where
did M's 150 ms go?

### Experiment F — more medium tasks

Press `c` (binary semaphore) and `d` (mutex). A second medium task N
(priority 4) arrives at 25 ms. How does H's blocking time change with the
semaphore, and with the mutex? Write the worst-case blocking time of H for
both, for *n* medium tasks of *C* ms each.

In 1997 the Mars Pathfinder lander kept resetting itself a few days after
landing. A high-priority bus task was blocked on a lock held by a
low-priority weather task, which in turn was preempted by medium-priority
communication tasks, until a watchdog reset the system. The fix, uploaded to
Mars, was to switch on priority inheritance for that lock.

## 3. Deadlock: `deadlock`

Upload `deadlock/deadlock.ino`. Task A takes m1 then m2; task B takes m2 then
m1. Both have priority 3.

### Experiment G — watch it happen

Press `a`. Both tasks wait forever (`portMAX_DELAY`), with 20 ms between their
two takes. After 1 s the sketch gives up and shows who holds which mutex and
what state each task is in. Draw the wait-for graph. Which of the four
Coffman conditions (mutual exclusion, hold and wait, no preemption, circular
wait) can you see in the output?

### Experiment H — detect and recover

Press `b`. Every take now gives up after 100 ms. A task that times out gives
back what it holds, waits a random 1–30 ms, and tries again. Both finish. How
many timeouts were there? Why the *random* wait? What goes wrong if both back
off for exactly the same time?

### Experiment I — prevent it

Press `c`. B now takes m1 before m2, the same order as A. Which Coffman
condition is gone? Why is this usually the preferred fix?

### Experiment J — the rare deadlock

Press `d`. No pause between the two takes anymore, 200 rounds per task, 50 ms
timeout. Now the deadlock only happens when a task switch falls exactly
between the two takes. Count the timeouts. Run it again. Why is this version
much more dangerous in a real product than the one in experiment G?

## 4. Questions

1. Experiment B passed with the wrong code. Name another way a race can stay
   hidden during testing and show up in the field.
2. The mutex in experiment C is much slower than the critical section. Why,
   and when is the mutex still the right choice?
3. Priority inheritance bounds H's blocking to one critical section. What does
   that say about how long critical sections should be?
4. The priority ceiling protocol is an alternative to inheritance. How does
   it work, and which problem from part 3 does it also prevent?
5. Instead of sharing a variable under a lock, L could send its results to H
   through a queue (`xQueueSend` / `xQueueReceive`). Would priority inversion
   still be possible? What does the queue cost instead?
6. On a two-core ESP32, experiment B would lose updates again. Why?
