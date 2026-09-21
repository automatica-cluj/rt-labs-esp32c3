# Lab 4 — Interrupts and deferred processing

The tick gives FreeRTOS a resolution of 1 ms. Anything faster, and anything
that must react to the outside world, goes through an interrupt. In this lab
you see how long it takes from a hardware event to the interrupt service
routine (ISR), and from the ISR to the task that does the real work. Then you
see what makes those times grow: critical sections, long ISRs, and a
missing `portYIELD_FROM_ISR`.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours.

Theory behind this lab (concept pages, in Romanian):
[Întreruperi și latență](https://github.com/automatica-cluj/rt-concepts/blob/main/4-interrupts/interrupts-and-latency.md),
[Procesare amânată](https://github.com/automatica-cluj/rt-concepts/blob/main/4-interrupts/deferred-processing.md),
[Secțiuni critice](https://github.com/automatica-cluj/rt-concepts/blob/main/4-interrupts/critical-sections.md),
[Debouncing](https://github.com/automatica-cluj/rt-concepts/blob/main/4-interrupts/debouncing.md).

The "Interrupts" section of
[../docs/freertos_concepts.md](../docs/freertos_concepts.md) introduces the
terms used below.

---

## 1. The measurement: `isr_latency`

Upload `isr_latency/isr_latency.ino`.

A hardware timer counts at 10 MHz (0.1 µs per count) and raises an alarm
every 0.997 ms, just under 1 ms (experiment B shows why). At the alarm it
restarts from 0. So the counter value the ISR reads as its very first action
is the time from the alarm to the ISR: the **interrupt latency**, measured by
the hardware itself and needing no wire or oscilloscope.

The ISR then wakes a **handler task** with a task notification
(`vTaskNotifyGiveFromISR`). The handler notes the cycle counter as soon as it
runs. The difference to the ISR's own cycle count is the **ISR-to-task
latency**.

```
alarm ──(interrupt latency)──> ISR ──(ISR-to-task latency)──> handler task
```

Each run takes 2000 samples, 2 s, and prints min, avg and max of both. At
reset the sketch also prints what one `timerRead()` costs. That cost is
included in every interrupt latency number.

## 2. Experiments

### Experiment A — the baseline (key `a`)

Press `a`. The handler has priority 10, and the ISR calls
`portYIELD_FROM_ISR` so the handler runs as soon as the ISR returns.

**Look at** the six numbers: min, avg and max of both latencies. Keep them
to compare with the other experiments.

**What it shows.** These are the best this chip and this code can do. Even
the minimum interrupt latency is several microseconds, not nanoseconds.
Before the first line of `on_alarm()` runs, the CPU has to finish the current
instruction, save the registers of the interrupted task, and go through the
interrupt dispatch code of ESP-IDF and the Arduino core, which then calls our
function.

### Experiment B — forgetting `portYIELD_FROM_ISR` (key `b`)

Press `b`. The ISR wakes the handler but does not ask for a context switch.
A background task at priority 1 computes all the time, as some task in a real
system usually does, so the interrupt always stops that task.

**Look at** the ISR-to-task latency next to A, and the line
`events shared a wake-up`.

**What it shows.** The interrupt latency is unchanged, but the ISR-to-task
latency is now large and spread out, with a maximum close to 1 ms. Without
the yield, the ISR returns to the background task, and the scheduler only
switches to the handler at the next tick. The handler waits anywhere from 0
to 1 ms, depending on where the alarm fell between two ticks.

Because the alarm comes slightly more often than the tick, now and then two
alarms fall between the same two ticks. The handler, woken only at the tick,
then finds both at once: that is the `shared a wake-up` count.

Without the background task the CPU would be idle, and the idle task gives
the CPU to the woken handler within a few microseconds. So on an idle bench
board a forgotten yield goes unnoticed, and it shows up later in a busy
system.

### Experiment C — the handler's priority (key `c`)

Press `c`. The handler now has priority 2. A busy task at priority 5 computes
3 ms and sleeps 2 ms, over and over.

**Look at** both latencies, and the `events shared a wake-up` line.

**What it shows.** The ISR still runs on time: interrupts do not care about
task priorities. The handler does not. It has to wait while the busy task
computes, up to about 3 ms. Meanwhile more interrupts arrive, so many events
share a wake-up. The notification counts them, so none is lost, but their
work is done together and late. The handler's priority has to be high enough
for the deadline of the event it handles.

### Experiment D — critical sections (key `d`)

Press `d`. The handler is back at priority 10. A task at priority 3 keeps
entering a critical section (`portENTER_CRITICAL`, interrupts off) for 200 µs
at a time.

**Look at** the interrupt latency next to A.

**What it shows.** This time it is the *interrupt* latency that grows, with a
maximum near 200 µs: an alarm that arrives while interrupts are off waits
until the critical section ends. A task at priority 3 just delayed an
interrupt and, through it, a task at priority 10. Priorities cannot protect
against this, because they only order tasks, and with interrupts off nothing
else runs. That is why critical sections must be kept very short.

### Experiment E — a long ISR somewhere else (key `e`)

Press `e`. A second timer fires every 3.7 ms, and its ISR "does the work"
right there: 300 µs of computation.

**Look at** both latencies next to A.

**What it shows.** Both grow, by up to about 300 µs. Both timer interrupts
have the same (low) interrupt level, and an interrupt cannot interrupt
another one of the same level. So when our alarm arrives during the other
ISR, ours waits until the other finishes. A long ISR delays everyone, not
only the code it belongs to.

### Experiment F — defer the work (key `f`)

Press `f`. Same second timer, but its ISR now only notifies a worker task at
priority 5, and the worker does the 300 µs of work.

**Look at** both latencies next to A and E.

**What it shows.** The latencies are back close to A. The other ISR is now
short, and the 300 µs of work runs in the worker task at priority 5, below
our handler at 10. The work did not go away, but now it is done at a
priority chosen for it. The less urgent work waits, not our handler. This is
deferred processing: keep the ISR short and hand the work to a task.

## 3. A real input: `button`

Upload `button/button.ino`. The BOOT button is on GPIO 9, with an interrupt on
every edge (`CHANGE`). The ISR only counts the edge, notes the time and wakes
a debounce task. The task waits until no edge has come for 20 ms, reads the
pin once, and decides whether the button was pressed or released. Each press
toggles the blue LED.

Press the button a few times, quickly and slowly, firmly and lightly. After
each burst the sketch prints how many edges it saw, how many real presses and
releases it counted, and the shortest time between two edges.

**Look at** the number of edges next to presses + releases, and the shortest
gap.

**What it shows.** There are usually more edges than presses and releases:
the contact **bounces**, closing and opening several times within a very
short time. If every edge counted as a press, the LED would toggle several
times per press and end up in a random state. The debounce task counts one
press per settled change. The 20 ms wait is in the task, not in the ISR,
because an ISR must be short: while it waits, other interrupts of its level
and every task are held up.

`v` prints a verdict line with the counts, and `r` resets them. Do not hold
BOOT while pressing RST: that starts the bootloader instead of the sketch.

Optional: set `kSettleMs = 1` and upload again. With so short a wait, a
contact that bounces for longer than 1 ms can be counted as more than one
press.
