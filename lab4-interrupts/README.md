# Lab 4 — Interrupts and deferred processing

The tick gives FreeRTOS a resolution of 1 ms. Anything faster, and anything
that must react to the outside world, goes through an interrupt. In this lab
you measure how long it takes from a hardware event to the interrupt service
routine (ISR), and from the ISR to the task that does the real work. Then you
find out what makes those times grow: critical sections, long ISRs, and a
missing `portYIELD_FROM_ISR`.

Time: about 2 hours.

Read the "Interrupts" section of
[../docs/freertos_concepts.md](../docs/freertos_concepts.md) first.

---

## 1. The measurement: `isr_latency`

Upload `isr_latency/isr_latency.ino`.

A hardware timer counts at 10 MHz (0.1 µs per count) and raises an alarm
every 1 ms. At the alarm it restarts from 0. So the counter value the ISR
reads as its very first action is the time from the alarm to the ISR: the
**interrupt latency**, measured by the hardware itself and needing no wire or
oscilloscope.

The ISR then wakes a **handler task** with a task notification
(`vTaskNotifyGiveFromISR`). The handler notes the cycle counter as soon as it
runs. The difference to the ISR's own cycle count is the **ISR-to-task
latency**.

```
alarm ──(interrupt latency)──> ISR ──(ISR-to-task latency)──> handler task
```

Each run takes 2000 samples, 2 s, and prints min, avg and max of both. At
reset the sketch also prints what one `timerRead()` costs. That cost is part
of every interrupt latency number, so subtract it in your head.

## 2. Experiments

### Experiment A — the baseline

Press `a`. The handler has priority 10, and the ISR calls
`portYIELD_FROM_ISR` so the handler runs as soon as the ISR returns.

Note all six numbers. These are the best this chip, this core and this code
can do. The minimum interrupt latency is several microseconds, not
nanoseconds. Read `on_alarm()`: what has to happen between the hardware alarm
and its first line?

### Experiment B — forgetting `portYIELD_FROM_ISR`

Press `b`. The ISR wakes the handler but does not ask for a context switch.

The interrupt latency is unchanged, but the ISR-to-task latency has become
huge and spread out. What is its maximum close to, and why? When does the
handler finally run?

### Experiment C — the handler's priority

Press `c`. The handler now has priority 2. A busy task at priority 5 computes
3 ms and sleeps 2 ms, over and over.

The ISR still runs on time. Does the handler? The line "events shared a
wake-up" counts interrupts whose handler was so late that the next interrupt
had already arrived. What happened to the work for those events? How would you
choose the handler's priority in a real system?

### Experiment D — critical sections

Press `d`. The handler is back at priority 10. A task at priority 3 keeps
entering a critical section (`portENTER_CRITICAL`, interrupts off) for 200 µs
at a time.

This time it is the *interrupt* latency that grows. What is its new maximum?
A task with priority 3 just delayed an interrupt and, through it, a task
with priority 10. Why can the priority not protect against this? What does
this mean for how long a critical section may be?

### Experiment E — a long ISR somewhere else

Press `e`. A second timer fires every 3.7 ms, and its ISR "does the work"
right there: 300 µs of computation.

Look at both latencies. Both timer interrupts get the same (low) interrupt
level, and an interrupt cannot interrupt another one of the same level. So
when our alarm arrives during the other ISR, ours waits until the other
finishes. The damage is not limited to the code that has the long ISR.

### Experiment F — defer the work

Press `f`. Same second timer, but its ISR now only notifies a worker task at
priority 5, and the worker does the 300 µs of work.

Compare with E. Our handler has priority 10, above the worker. Do the
latencies go back to A? Which task pays for the 300 µs now, and is that the
right one?

## 3. A real input: `button`

Upload `button/button.ino`. The BOOT button is on GPIO 9, with an interrupt on
every edge (`CHANGE`). The ISR only counts the edge, notes the time and wakes
a debounce task. The task waits until no edge has come for 20 ms, reads the
pin once, and decides whether the button was pressed or released. Each press
toggles the blue LED.

Press the button a few times, quickly and slowly, firmly and lightly. After
each burst the sketch prints how many edges it saw, how many real presses and
releases it counted, and the shortest time between two edges.

- More edges than presses + releases means the contact **bounced**. How many
  edges per press did you get? How short was the shortest gap?
- If every edge counted as a press, what would a user see?
- Why is the 20 ms wait in the task and not in the ISR?
- Try `kSettleMs = 1`. Does it still debounce?

`v` prints a verdict line with the counts, and `r` resets them. Do not hold
BOOT while pressing RST: that starts the bootloader instead of the sketch.

## 4. Questions

1. Draw the timeline of experiment A from the hardware alarm to the handler
   finishing, with the approximate times you measured.
2. Why does `portYIELD_FROM_ISR` take the `woken` flag instead of always
   switching?
3. From experiments D and E: write down a rule for the maximum length of an
   ISR and of a critical section in a system whose tightest interrupt latency
   requirement is 50 µs.
4. The handler task in C missed events. Give two ways to make sure no event is
   lost even when the handler is late. (Hint: what does a notification count,
   and what could a queue hold?)
5. The debounce task uses the time of the *last* edge. Could you debounce
   inside the ISR instead, by ignoring edges closer than 20 ms to the previous
   one? What would you lose?
