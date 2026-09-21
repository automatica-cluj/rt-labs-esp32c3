# FreeRTOS on the ESP32-C3: the concepts these labs use

The Arduino core for the ESP32 is built on ESP-IDF, and ESP-IDF runs
FreeRTOS. Your sketch is not alone on the chip: `setup()` and `loop()` run
inside a FreeRTOS task, next to a few tasks the system starts itself. The labs
call the FreeRTOS API directly, so this page explains what is going on
underneath.

## Tasks and priorities

A **task** is a function with its own stack that the scheduler can stop and
resume. You create one with

```cpp
xTaskCreate(function, "name", stack_bytes, argument, priority, &handle);
```

`rt_start_task()` in `rt.h` wraps this and stops the sketch if it fails.

Priorities are numbers from 0 to 24. **A higher number means a higher
priority.** The scheduler follows one rule: *the highest-priority task that is
ready runs.* If a higher-priority task becomes ready, it preempts the running
one at once. That is **fixed-priority preemptive scheduling**, and there is
nothing else to it.

The ESP32-C3 has **one core**, so exactly one task runs at any moment.

Tasks the system starts, on this board with the Arduino core:

| Task | Priority | What it does |
|---|---|---|
| `IDLE` | 0 | runs when nothing else can; frees memory of deleted tasks |
| `loopTask` | 1 | runs your `setup()` once, then `loop()` forever |
| `Tmr Svc` | 1 | FreeRTOS software timers |
| `sys_evt` | 20 | the system event loop (Wi-Fi events), only when Wi-Fi is on |
| `esp_timer` | 22 | callbacks of `esp_timer` (microsecond timers) |
| `wifi` | 23 | the Wi-Fi driver, only when Wi-Fi is on |

The labs keep their own tasks between 1 and 20.

## Task states

| State | Meaning |
|---|---|
| Running | has the CPU (one task at a time) |
| Ready | could run, but a task of higher or equal priority has the CPU |
| Blocked | waits for time (`vTaskDelay`) or an event (semaphore, notification, queue) |
| Suspended | stopped by `vTaskSuspend` until someone resumes it |

`vTaskList()` prints this table for every task (Lab 2).

## The tick

A timer interrupt fires every **1 ms** (`configTICK_RATE_HZ = 1000`). At each
tick the scheduler wakes tasks whose delay has run out, and rotates between
ready tasks of **equal** priority (time slicing). All FreeRTOS delays and
timeouts are counted in ticks.

- `vTaskDelay(n)` blocks until *n* ticks from now. The first of those ticks can
  be almost immediately, so `vTaskDelay(1)` lasts anything between 0 and 1 ms.
  Arduino's `delay(ms)` is `vTaskDelay` underneath.
- `xTaskDelayUntil(&last, n)` blocks until the absolute tick `last + n`, then
  sets `last` to that tick. This is how a periodic task avoids drift (Lab 1).
- For anything finer than a tick you need a hardware timer and its interrupt
  (Lab 4).

## Interrupts

An **interrupt service routine (ISR)** runs when hardware asks for it,
whatever task is running, and even ahead of the highest-priority task. Rules:

- Keep it short. While it runs, no task runs, and other interrupts of the same
  level wait.
- Never block in an ISR: no `delay`, no `xSemaphoreTake` with a timeout, no
  `Serial.print`.
- Only call API functions that end in `FromISR`.
- If the ISR woke a task with a higher priority than the one it interrupted,
  call `portYIELD_FROM_ISR(woken)` so the switch happens as the ISR returns,
  not at the next tick.
- Mark the function `ARDUINO_ISR_ATTR`.

The usual pattern is **deferred processing**: the ISR records what happened
and wakes a task, and the task does the work at a priority you choose.

## Sharing data

| Tool | Use it for | Notes |
|---|---|---|
| **Critical section** `portENTER_CRITICAL(&mux)` | a few instructions on shared data | turns interrupts off; nothing else runs, not even ISRs. Keep it to microseconds |
| **Mutex** `xSemaphoreCreateMutex()` | a resource held for a while | has an owner and **priority inheritance** |
| **Binary semaphore** `xSemaphoreCreateBinary()` | signalling "it happened" | no owner, **no** priority inheritance. Using one as a lock invites priority inversion (Lab 5) |
| **Counting semaphore** | counting events or free slots | |
| **Task notification** `xTaskNotifyGive` / `ulTaskNotifyTake` | waking one particular task, often from an ISR | the fastest option; no object to create |
| **Queue** `xQueueSend` / `xQueueReceive` | passing data by copy between tasks | the receiver owns its copy; no lock needed |
| an atomic add (`__sync_fetch_and_add`) | a single counter or flag | the C3 has no atomic instructions, so the compiler briefly disables interrupts instead |

## Linux PREEMPT_RT equivalents

If you know the Linux labs, this table maps the ideas.

| Linux (POSIX) | FreeRTOS | Difference |
|---|---|---|
| `pthread_create` + `SCHED_FIFO` | `xTaskCreate` | every task is "real-time"; no policy to choose |
| priority 1-99, higher wins | 0-24, higher wins | |
| `SCHED_RR` time slice | time slicing between equal priorities, one tick | always on here |
| `clock_nanosleep(TIMER_ABSTIME)` | `xTaskDelayUntil` | tick resolution (1 ms), not nanoseconds |
| `mlockall` | not needed | there is no virtual memory to page out |
| CPU pinning | not needed | one core |
| RT throttling (95 %) | task watchdog | throttling slows a runaway task; the watchdog resets the chip |
| `PTHREAD_PRIO_INHERIT` mutex | `xSemaphoreCreateMutex` | inheritance is always on for a FreeRTOS mutex |
| `pthread_mutex_timedlock` | `xSemaphoreTake(m, timeout)` | |
| signal handler / IRQ thread | ISR + task notification | |
