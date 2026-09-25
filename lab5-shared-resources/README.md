# Lab 5 — Shared resources: races, priority inversion, deadlock

As soon as two tasks share data, three things can go wrong. Updates get lost
(a race). A high-priority task ends up waiting for a low-priority one, and
then for a medium one that has nothing to do with it (priority inversion).
Or two tasks wait for each other forever (deadlock). You see each of them
happen on the board, and then see it fixed.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours.

Theory behind this lab (concept pages, in Romanian):
[Condiții de cursă](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/race-conditions.md),
[Excludere mutuală](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/mutual-exclusion.md),
[Inversiunea de prioritate](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/priority-inversion.md),
[Deadlock](https://github.com/automatica-cluj/rt-concepts/blob/main/5-shared-resources/deadlock.md).

"Sharing data" in [../docs/freertos_concepts.md](../docs/freertos_concepts.md)
introduces the mutex and the critical section used below.

---

## 1. Lost updates: `race`

Upload `race/race.ino`. Two tasks each add 1 to a shared counter a million
times. The right answer is 2 000 000.

`g_counter = g_counter + 1` compiles to three instructions: load, add, store.
If the scheduler switches to the other task between the load and the store,
the first task later stores a stale value, and everything the other task
added in the meantime is gone.

Each variant prints the final count, how many updates were lost, and how long
the run took.

### Experiment A — the race (key `a`)

Press `a`: both tasks at priority 3, no protection. Run it two or three times.

**Look at** the `lost` count in each run.

**What it shows.** Updates are lost, and the number changes from run to run,
because it depends on exactly where the tick interrupt falls. A single bad
switch loses many updates at once: the tick lets each task run for a whole
millisecond, thousands of increments, and when the interrupted task resumes,
its stale store overwrites all of them.

### Experiment B — the race that hides (key `b`)

Press `b`: the same code, but priorities 3 and 4.

**Look at** the `lost` count.

**What it shows.** Nothing is lost, but the code is not fixed. The task at
priority 4 never blocks, so it runs all its million increments before the
other one starts. The two never overlap, so the race cannot happen. As soon
as the higher task has to wait for something in the middle (a delay, a
queue, a lock), the two interleave and updates are lost again. A test that
passes does not prove the code is correct.

### Experiment C — three fixes (keys `c`, `d`, `e`)

Press `c` (mutex), `d` (critical section) and `e` (atomic add).

**Look at** the count and the time of each.

**What it shows.** All three give 2 000 000, at different costs. The mutex
makes a call into the kernel for every increment. The critical section
switches interrupts off around the increment. The C3 has no atomic
instructions, so the atomic add also switches interrupts off briefly. If an
interrupt handler also changes the counter, only the critical section or the
atomic add can be used, because an interrupt handler is not allowed to wait
for a mutex.

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

### Experiment D — binary semaphore (key `a`)

Press `a`. The lock is a binary semaphore.

**Look at** who runs in the timeline from 20 ms on, and the `blocked` time
of H.

**What it shows.** At 20 ms M starts and preempts L, because M has the higher
priority. L cannot finish its critical section, so H keeps waiting. H is
blocked for the rest of L's work plus all of M's 150 ms. H has the highest
priority, yet it waits for M, a task that does not use the lock. This is
priority inversion.

### Experiment E — mutex with priority inheritance (key `b`)

Press `b`. The lock is now a FreeRTOS mutex.

**Look at** the timeline again, the priority L ran at while holding the lock,
and H's `blocked` time.

**What it shows.** While H waits, L runs at H's priority (5). M cannot preempt
it, so L finishes its critical section and hands the lock to H. H waits only
for the rest of L's critical section. M's 150 ms run after H is done. This is
priority inheritance.

### Experiment F — more medium tasks (keys `c`, `d`)

Press `c` (binary semaphore) and `d` (mutex). A second medium task N
(priority 4) arrives at 25 ms.

**Look at** H's `blocked` time in both runs, next to D and E.

**What it shows.** With the semaphore, H's blocking grows by N's 150 ms as
well: every medium task added makes it longer, with no limit. With the mutex
it stays the same as in E, bounded by L's critical section, however many
medium tasks there are.

In 1997 the Mars Pathfinder lander kept resetting itself a few days after
landing. A high-priority bus task was blocked on a lock held by a
low-priority weather task, which in turn was preempted by medium-priority
communication tasks, until a watchdog reset the system. The fix, uploaded to
Mars, was to switch on priority inheritance for that lock.

## 3. Deadlock: `deadlock`

Upload `deadlock/deadlock.ino`. Task A takes m1 then m2; task B takes m2 then
m1. Both have priority 3.

### Experiment G — watch it happen (key `a`)

Press `a`. Both tasks wait forever (`portMAX_DELAY`), with 20 ms between their
two takes. After 1 s the sketch gives up and shows who holds which mutex and
what state each task is in.

**Look at** the lines that say who holds m1 and m2, and the state of A and B.

**What it shows.** A holds m1 and waits for m2, B holds m2 and waits for m1.
Neither can continue:

```
A --waits for--> m2 --held by--> B --waits for--> m1 --held by--> A
```

All four conditions for a deadlock are present: a mutex has one owner at a
time (mutual exclusion), each task keeps one mutex while it waits for the
other (hold and wait), nobody can take a mutex away from its owner (no
preemption), and the waits form a cycle (circular wait).

### Experiment H — detect and recover (key `b`)

Press `b`. Every take now gives up after 100 ms. A task that times out gives
back what it holds, waits a random 1–30 ms, and tries again.

**Look at** the `timeouts` count and the `both finished` line.

**What it shows.** Both tasks finish, after a few timeouts. The wait is random
so that the two tasks try again at different moments. If both waited exactly
the same time, they would take their first mutex at the same moment again,
and could repeat the same deadlock over and over.

### Experiment I — prevent it (key `c`)

Press `c`. B now takes m1 before m2, the same order as A.

**Look at** the verdict and the timeouts.

**What it shows.** No deadlock and no timeouts. Whoever gets m1 first also gets
m2, so the waits can no longer form a cycle. Taking locks in one fixed order
removes the circular wait, and it costs nothing at run time, which is why it
is usually the preferred fix.

### Experiment J — the rare deadlock (key `d`)

Press `d`. No pause between the two takes anymore, 200 rounds per task, 50 ms
timeout. Run it two or three times.

**Look at** the `timeouts` count in each run.

**What it shows.** The deadlock now happens only when a task switch falls
exactly between the two takes, so there are few timeouts, and a different
number each run. This is the dangerous kind: it can pass every test in the
lab and show up months later in a product.
