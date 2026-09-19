# Lab 1 — Time, latency and jitter

Get the board running, find out what the clocks on it can measure, and see how
regular a periodic task really is: relative against absolute sleeps, the
effect of priorities, and the interference you get from a radio sharing the
same CPU.

Time: about 2 hours, of which the first 30 minutes are setup.

Theory behind this lab (concept pages, in Romanian):
[Ce înseamnă timp real](../concepts/00-what-is-real-time.md),
[Ceasuri și rezoluție](../concepts/1-time/clocks-and-resolution.md),
[Latență și jitter](../concepts/1-time/latency-and-jitter.md),
[Eliberare periodică](../concepts/1-time/periodic-release.md).

---

## 1. Setup

Follow [../docs/setup.md](../docs/setup.md) up to and including step 3. When
`clocks` prints its first lines, you are ready.

Read [../common/rtlab.h](../common/rtlab.h) once. Every sketch includes a copy
of it: `now_us()`, `cycles()`, `burn_us()`, `Stats`, `start_task()` and
`verdict()` come from there.

## 2. Clocks: `clocks`

Upload `clocks/clocks.ino`, open the serial monitor, press RST. Press `r` to
run it again.

The sketch prints five parts:

1. **Resolution.** The smallest step each clock can show. `millis()` moves in
   whole milliseconds, `micros()` and `esp_timer_get_time()` in microseconds,
   and the cycle counter in 1/160 µs.
2. **Cost.** How many CPU cycles one reading takes. A clock that costs more to
   read than the thing you time is useless for that job.
3. **Sleeping.** `delay(1)` and `vTaskDelay(1)` do not mean "1000 µs". Look at
   the minimum. FreeRTOS counts delays in ticks of 1 ms, and "one tick from
   now" can be a few microseconds away if you call it just before a tick.
4. **Busy waiting.** `delayMicroseconds()` and `burn_us()` both keep the CPU.
   Compare their accuracy. `burn_us()` is how every later lab models
   "computation time".
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

### Experiment A — relative sleep

Press `a`. The task sleeps with `vTaskDelay(10)` *after* its work.

Look at the interval and the drift. Explain the interval from T and C. What
would a clock that "ticks" with this task show after one hour?

### Experiment B — absolute sleep

Press `b`. The task uses `xTaskDelayUntil(&last, 10)`: "wake me at
`last + 10`", measured from the previous *release*, not from *now*.

The drift should be gone and the jitter small. Where does the remaining jitter
come from? (Hint: what else runs every millisecond?)

### Experiment C — a higher-priority task in the way

Press `c`. A "hog" task at priority 10 computes 2 ms, sleeps 7 ms, forever.
Its period is not a multiple of 10 ms, so sometimes it is running exactly when
our task should wake up.

Compare the lateness histogram with B. What is the largest lateness you would
expect from the hog's parameters? Does the measurement agree?

### Experiment D — raise our priority

Press `d`. Same hog, but our task now has priority 15. What changed, and what
happened to the hog? Could the hog now miss *its* deadlines?

### Experiment E — the radio

Press `e`. No hog. Instead the Wi-Fi radio scans for networks. The Wi-Fi driver
runs as a task with priority 23 and in interrupts, both above any task you
create.

Compare the jitter with B and D. Our task has priority 15, yet it is delayed.
Why can raising its priority not fix this? What would you do in a real device
that needs both Wi-Fi and a tight control loop? (The ESP32 and ESP32-S3 have
two cores. How would that help?)

### Optional — plot it

After a run, press `p` and save the serial output to a file (copy it from the
monitor, or use `arduino-cli monitor ... | tee run.log`, see setup.md). Then:

```
python3 tools/plot_serial.py run.log
```

Do this for B, C and E and put the three plots side by side.

## 4. Questions

Write your answers in a short text file.

1. Why does `vTaskDelay(1)` sometimes return in much less than 1 ms? What does
   that mean for code that "waits 1 ms to let the hardware settle"?
2. In experiment A the interval is larger than T. Give the formula. What
   happens in A if C is larger than T?
3. `xTaskDelayUntil` fixes the drift. What does it do if a job takes longer
   than T, so the next release is already in the past?
4. In C, lateness is caused by a higher-priority task. In B, it is caused by
   something that is not a task at all. What is it, and how large is it?
5. The lateness histogram in C has a long tail. Why is the *maximum* the
   number a real-time engineer cares about, and not the average?
6. Printing one line costs how many periods of a 1 kHz control loop?

## Rules for these labs

- Every experiment ends with a `VERDICT:` line. Quote it in your report.
- If the board stops responding, see the recovery steps in setup.md. Nothing
  you do in these labs can damage it.
