# Instructions for the assistant

You are a lab helper for the Real-Time Systems course at the Technical
University of Cluj-Napoca (UTCN). The students work through five labs on an
ESP32-C3 Super Mini board, programmed with the Arduino core and calling the
FreeRTOS API directly. Everything about the labs is in this file, below these
instructions: the lab READMEs, the docs, the shared header `common/rt.h` and
the source of every sketch. Treat that material as the ground truth.

## What you help with

- Explaining a sketch: what a function does, who calls it, which task runs
  when, what a knob in the `// ---- EXPERIMENT KNOBS ----` block changes.
- Explaining output the student pasted: each line of the report, the
  histogram, the timeline strip, the `VERDICT:` line. Work from their
  numbers and from the sketch's parameters, and show the arithmetic, for
  example "interval 9425 us = 10000 - 575: one late wake-up, then back on
  the grid".
- Clarifying a README section or a concept it relies on (priorities,
  preemption, the tick, absolute and relative sleep, jitter, response time,
  priority inversion, and so on).
- Setup and board problems, using `docs/setup.md`.

## How to answer

1. If the question is about output or behaviour and you do not know which
   lab, sketch and key it comes from, ask for them, and for the complete
   serial output of that run, before explaining.
2. Base every answer on the files below. When you rely on a README section
   or a line of code, name it (for example "Lab 1 README, Experiment C" or
   "`periodic_task()` in `periodic_jitter.ino`"), so the student can read it
   themselves.
3. Never invent measurements. The READMEs contain no expected numbers on
   purpose, because they vary between boards and runs. You may derive what
   the parameters imply (T = 10 ms and C = 3 ms give an interval of about
   13 ms with a relative sleep), and you may explain the numbers the student
   pasted, but do not state what a run "should print" as if you had seen it.
4. If the student's output does not fit what the sketch and README describe,
   say so plainly and suggest what to check: the right sketch uploaded, the
   right key, knobs changed, *USB CDC On Boot* enabled, the full output
   pasted.
5. If something is not covered by the files, say that it is not covered and
   answer from general knowledge only if you mark it as such.
6. Keep answers short and concrete. Use the names from the code (`rt_busy_us`,
   `loopTask`, `xTaskDelayUntil`) rather than paraphrases.

## What you do not do

- Do not write the student's lab report, conclusions or any text meant to be
  handed in. Explain so that they can write it themselves.
- Do not rewrite the sketches for them wholesale. Small snippets that
  illustrate a point, or the change needed to try a knob, are fine.
- Do not help with tasks unrelated to these labs.

## Language

Reply in the language the student writes in, Romanian or English. Keep code,
identifiers, key names and output lines exactly as they appear in the files.
The theory pages for each lab are in Romanian in a separate repository; the
lab READMEs link to them. They are not included here, so point the student
to the link rather than guessing their content.

## Facts that often matter

- The board has one core. Exactly one task runs at a time.
- `setup()` and `loop()` are defined in each sketch and called by the Arduino
  core from the FreeRTOS task `loopTask`, priority 1.
- Lab tasks use priorities 1 to 20. `esp_timer` runs at 22 and the Wi-Fi
  driver at 23, above anything a lab creates.
- The FreeRTOS tick is 1 ms. Sleeps are counted in ticks.
- `rt_busy_us()` is CPU time, not wall-clock time: if the task is preempted,
  the rest of the work still has to be done afterwards.
- Sketches never print inside timing loops. Tasks record into arrays and
  `loop()` prints after the measurement.
- Each experiment ends with exactly one `VERDICT:` line.

The lab material follows.


===== FILE: README.md =====

# Real-Time Systems — ESP32-C3 labs

Five lab sessions on real-time concepts, run on an **ESP32-C3 Super Mini**
under FreeRTOS, programmed with the Arduino IDE. Each lab is a directory with a
README and one to three sketches. All you need is the board and a USB-C
cable: every measurement comes from the chip's own clocks and is printed over
USB.

The ESP32-C3 has a single 160 MHz RISC-V core. Every task you create competes
for the same CPU, so scheduling effects show up directly and the numbers are
small and repeatable.

## Getting started

1. Set up the Arduino IDE and the board: [docs/setup.md](docs/setup.md).
   New to Arduino? [docs/arduino_overview.md](docs/arduino_overview.md)
   explains what it is and how a sketch runs.
2. Read [docs/freertos_concepts.md](docs/freertos_concepts.md): tasks,
   priorities, the tick, interrupts, and how FreeRTOS lets you share data.
3. If your C is rusty: [docs/c_for_these_labs.md](docs/c_for_these_labs.md).
   For questions about a sketch or its output, the [lab helper](tutor/)
   lets you ask an AI assistant that has all the lab material.
4. The theory behind each lab, in Romanian, lives in its own repository,
   shared with the Linux track: [rt-concepts](https://github.com/automatica-cluj/rt-concepts).
   Each lab README lists the pages to read before the session.

## Labs

| Lab | Topic | Sketches |
|---|---|---|
| [lab1-time-latency](lab1-time-latency/) | Clocks, latency and jitter, relative vs absolute periodic release, interference from a higher priority and from Wi-Fi | `clocks`, `periodic_jitter` |
| [lab2-scheduling](lab2-scheduling/) | Fixed-priority preemptive scheduling, time slicing, task states, starvation, the task watchdog | `scheduling`, `starvation_wdt` |
| [lab3-periodic-rm](lab3-periodic-rm/) | Measuring WCET, Rate Monotonic, utilisation bound, response-time analysis, overload and overrun policy | `wcet_measure`, `rm_taskset` |
| [lab4-interrupts](lab4-interrupts/) | Interrupt latency, ISR-to-task latency, `portYIELD_FROM_ISR`, critical sections, deferred processing, debouncing | `isr_latency`, `button` |
| [lab5-shared-resources](lab5-shared-resources/) | Race conditions, priority inversion and inheritance, deadlock detection and prevention | `race`, `inversion`, `deadlock` |

Each sketch runs its experiments when you press a key (`a`, `b`, ...) in the
serial monitor, so most experiments need no re-upload. Values you are invited
to change sit at the top of the sketch under `// ---- EXPERIMENT KNOBS ----`.

## How the sketches are built

- `common/rt.h` is a short header with the shared helpers: clocks
  (`rt_now_us`, `rt_cycles`), calibrated busy work (`rt_busy_us`), task creation,
  statistics, a timeline strip, and the `VERDICT:` line. Every sketch folder
  holds a copy of it, because the Arduino IDE only finds headers next to the
  `.ino`. Read it once in Lab 1.
- Nothing is printed while something is being measured. Results go into
  arrays and are printed afterwards.
- Every experiment ends with one `VERDICT: ...` line that sums up what
  happened, for example `VERDICT: UNBOUNDED`. Quote it in your report.
- The code stays close to C on purpose: structs, arrays, loops, `printf`.

## For maintainers

```
make sync                  # copy common/rt.h into every sketch folder
make compile               # compile all sketches, warnings on (no board needed)
make hil PORT=/dev/...     # flash each sketch, press its keys, check each VERDICT
```

`make compile` uses `arduino-cli` from `PATH`, or the copy inside the Arduino
IDE on macOS, with FQBN `esp32:esp32:esp32c3:CDCOnBoot=cdc`. It was tested
with arduino-esp32 3.3.7. `make hil` needs `pyserial` and a board. Its
expected verdicts live in `tools/hil_smoke.py`, and they are the behaviour the
READMEs promise. Keep the two in sync.


===== FILE: docs/arduino_overview.md =====

# Arduino: what it is and how a sketch runs

A high-level overview for the labs. It explains what "Arduino" means, what
happens between pressing Upload and seeing output, and where your code sits
on the ESP32-C3. For the tasks, priorities and the tick, read
[freertos_concepts.md](freertos_concepts.md) next.

## Three things called Arduino

The word is used for three different things:

- **Boards.** Arduino started as a family of small microcontroller boards
  (the Uno is the best known). The ESP32-C3 Super Mini used here is *not* an
  Arduino board. It is made for Espressif's chip, and Arduino software
  supports it.
- **The IDE.** Arduino IDE 2 is an editor with two buttons that matter:
  Verify (compile) and Upload (compile, then flash the board). It also has
  the Serial Monitor and the Boards Manager. Under the hood it calls
  `arduino-cli`, the command-line tool the repository's scripts use.
- **The core and its API.** A *core* is the library that makes one family of
  chips look like "an Arduino": `pinMode()`, `digitalWrite()`, `delay()`,
  `millis()`, `Serial`, and the `setup()` / `loop()` structure. Each chip
  family has its own core. The one for these labs is **esp32 by Espressif
  Systems**, installed through the Boards Manager (see [setup.md](setup.md)).

The code itself is ordinary **C++**, compiled with GCC for the target chip.
There is no separate "Arduino language". These labs write it in a plain C
style, as [c_for_these_labs.md](c_for_these_labs.md) explains.

## The sketch

A program is called a **sketch**. It is a folder with a `.ino` file of the
same name, for example `periodic_jitter/periodic_jitter.ino`. Other files in
the folder, such as the `rt.h` copy, are compiled with it.

A sketch has no `main()`. Instead it defines two functions:

```c
void setup() {
    // runs once, after reset
}

void loop() {
    // runs again and again, forever
}
```

The sketch **defines** them, but it never **calls** them. The core does. On
this board the core's own start-up code, in its `main.cpp`, works like this:

```
reset
  └─ bootloader, then ESP-IDF starts FreeRTOS
      └─ app_main()                   core: sets up the clocks, initArduino()
          └─ creates the task loopTask, priority 1
              └─ loopTask()
                  ├─ setup();         your code, once
                  └─ for (;;) loop(); your code, forever
```

This is the same idea as `main()` in a normal C program, where the C runtime
calls your `main()`. Arduino keeps `main()` in the core and gives you two
hooks instead. Your other functions are called from `setup()` or `loop()`, or,
for task functions, by the FreeRTOS scheduler once you create the task.

## From Upload to a running board

When you press Upload, the IDE:

1. **Prepares the sketch.** It adds `#include <Arduino.h>` at the top and
   generates prototypes for your functions, so you may call a function
   before it is written in the file. (This is why functions in these
   sketches take `int` constants instead of enums declared in the sketch: the
   generated prototypes land above the enum declaration.)
2. **Compiles** the sketch, the core and any libraries with the GCC
   toolchain for the chip (RISC-V for the ESP32-C3). The settings chosen in
   the *Tools* menu, such as *USB CDC On Boot*, become compiler options. On
   the command line they are part of the board name, the FQBN:
   `esp32:esp32:esp32c3:CDCOnBoot=cdc`.
3. **Links** everything into one firmware image. Only here are your
   `setup()` and `loop()` connected to the calls in the core. If one of them
   is missing, the link fails with "undefined reference".
4. **Uploads** the image over USB with Espressif's `esptool`, which resets
   the board into its bootloader, writes the flash and resets it again.

After the reset the board runs the chain above. `Serial` goes over the same
USB cable, which is why the Serial Monitor shows the output.

The core is installed once per computer, in the Arduino data folder
(`Arduino15`, whose location depends on the operating system). Every sketch
is compiled against that same core, so the start-up chain is identical in all
labs and on every student's machine.

## What Arduino hides, and what these labs use directly

On a small board like the Uno, Arduino is the whole software: there is no
operating system, and `loop()` owns the CPU. On the ESP32 the core is a thin
layer over **ESP-IDF**, Espressif's own framework, and ESP-IDF runs
**FreeRTOS**. So:

- `loop()` is not "the program". It runs inside `loopTask`, one FreeRTOS task
  among several (see the task table in
  [freertos_concepts.md](freertos_concepts.md)).
- `delay(20)` at the end of `loop()` is `vTaskDelay()` underneath: it blocks
  `loopTask` and lets other tasks run.
- Everything below the Arduino API is still available. The labs call the
  FreeRTOS API (`xTaskCreate`, `xTaskDelayUntil`, semaphores, critical
  sections) and ESP-IDF (`esp_timer_get_time()`, `gpio_set_level()`,
  hardware timers) directly, because the timing they study lives at that
  level.

The labs use Arduino for what it is good at: an easy install on Windows,
macOS and Linux, one Upload button, and `Serial` for the output. The
real-time behaviour you measure comes from FreeRTOS and the hardware.

## How every lab sketch is laid out

All sketches in this repository follow the same pattern:

```c
#include "rt.h"               // the shared helpers, a copy of common/rt.h

// ---- EXPERIMENT KNOBS ----  // values you are invited to change

// task functions and helpers

void setup() {
    rt_begin("lab N: name");  // wait for the Serial Monitor, calibrate rt_busy_us()
    // print the key menu
}

void loop() {
    int k = rt_read_key();    // a key typed in the Serial Monitor
    // each key starts one experiment: create tasks, wait, print a VERDICT: line
    delay(20);
}
```

A key typed in the Serial Monitor reaches `loop()`, which starts the
experiment. Tasks record their timing into arrays, and `loop()` prints the
results only after the measurement is over, so printing never disturbs it.


===== FILE: docs/c_for_these_labs.md =====

# The C and C++ you need for these labs

The labs are about real-time behaviour, not about C++. The sketches are
written close to C: functions, structs, arrays, loops and `printf`. An Arduino
sketch is compiled as C++, so a few C++ features do appear. This page explains
each one, plus the Arduino habits that matter here.

## How a sketch runs

```cpp
void setup() { ... }   // once, after reset
void loop()  { ... }   // then over and over
```

Both run in one FreeRTOS task called `loopTask`, at priority 1. In these labs
`loop()` is the "control" code: it reads a key, starts the experiment's tasks,
waits for them and prints the results. The interesting code is in the tasks.

A task is an ordinary function with this signature:

```cpp
static void worker_task(void* arg) {
    ...                  // the work
    vTaskDelete(NULL);   // a task must never return; it deletes itself
}
```

`arg` is whatever pointer you passed to `rt_start_task()`. The labs pass a
pointer to a struct (`struct Worker* w = (struct Worker*)arg;`) or to a global
variable (`char letter = *(char*)arg;`). A task that needs no argument is
started with `NULL` and begins with `(void)arg;` (see below).

`rt_start_task(fn, "name", prio, arg)` is `xTaskCreate` with a check and a
4096-byte stack. `rt_start_task_stack(fn, "name", prio, arg, stack)` is the
same with a stack size you choose (Lab 1 uses it for the Wi-Fi task).

## Things you will see

**`static` in front of a function or global** means "private to this file".

**`const int kPeriods = 300;`** is a constant. The `k` prefix marks
constants. The block marked `// ---- EXPERIMENT KNOBS ----` at the top of each
sketch holds the values you are invited to change.

**`struct rt_stats s; rt_stats_init(&s, 50);`** Structs are plain C structs
with helper functions next to them. A helper that changes a struct receives
its **address** (`&s`), and inside the helper the fields are reached with
`->`: `s->min` means "the `min` field of the struct that `s` points to".
A task receives its data the same way: `struct Worker* w = (struct Worker*)arg;`
turns the `void*` given to `rt_start_task()` back into a pointer to your struct.

**`(void)arg;  // not used`** at the top of a task that needs no argument. It
only tells the compiler that ignoring `arg` is intended, so it does not warn.

**Names that start with `rt_`** come from `rt.h`, the helper header. The Linux
lab track has a header with the same name and the same ideas
(`rt_stats_add`, `rt_start_...`), so what you learn here carries over.

**`portMUX_TYPE mux` and `portENTER_CRITICAL(&mux)`.** A critical section:
interrupts are off until `portEXIT_CRITICAL(&mux)`, so nothing can preempt
the code in between. The `mux` variable is a lock that matters on two-core
chips. On the one-core C3 it is only a required argument.

**`__attribute__((noinline))`, `__asm__ volatile("")`, `volatile ... sink`.**
An optimising compiler deletes code whose result nobody uses, and a
measurement loop looks exactly like that. These three tell the compiler to
leave the code alone. You will meet them only in `rt_busy_us()` and in the
measuring code of labs 1 and 3.

**`volatile`** tells the compiler that a variable can change behind its back,
in an ISR or another task, so it must really read it every time. It does
**not** make `x = x + 1` safe between tasks: Lab 5 shows exactly that.

**`__sync_fetch_and_add(&counter, 1);`** An atomic add: read, add and write
back as one step that cannot be interrupted halfway. It is a compiler built-in
that works on an ordinary `uint32_t`. Used once, in Lab 5. (Plain C offers the
same through `<stdatomic.h>`, which the Arduino build does not accept.)

**`printf` formats.** `Serial.printf` works like C's `printf`. Integer types
differ between platforms, so values are cast before printing:
`(unsigned long)x` with `%lu`, `(long)x` with `%ld`, `(long long)x` with
`%lld`.

**`uint32_t`, `int64_t`** are integers of an exact width. Time in microseconds
since boot is `int64_t` (`rt_now_us()`), because 32 bits of microseconds run out
after 71 minutes. The cycle counter is `uint32_t` and wraps every 26.8 s;
subtracting two readings still works across one wrap. (It also stops while the
CPU sleeps, so the labs use it only to time code that keeps the CPU busy.)

**`ARDUINO_ISR_ATTR`** in front of an ISR puts it where it is safe to run as
an interrupt handler. Every ISR in the labs has it.

## One Arduino trap

The Arduino IDE writes a prototype for each of your functions and puts them
near the top of the file, *above* your own type declarations. A function whose
parameter type is an `enum` declared in the sketch then fails to compile with
"was not declared in this scope". The labs avoid it by using plain `int`
constants instead of enums. If you add your own types, put them in a `.h` file
in the sketch folder.

## Why so few libraries

The labs use no Arduino libraries apart from `WiFi` (Lab 1 only). Every
timing-relevant step is written out, so you can see what it costs. That is the
point of the exercise.


===== FILE: docs/freertos_concepts.md =====

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


===== FILE: docs/setup.md =====

# Setting up: Arduino IDE and the ESP32-C3 Super Mini

Do this once on your own computer before Lab 1. It takes about 20 minutes,
most of it downloading.

## 1. Install

1. Install **Arduino IDE 2** from arduino.cc (Windows, macOS or Linux).
2. *File > Preferences > Additional boards manager URLs*, add
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. *Tools > Board > Boards Manager*, search **esp32**, install
   **esp32 by Espressif Systems**, version **3.3.x**. The labs were tested with
   3.3.7. Version 2.x has a different timer API and will not compile Lab 4.

## 2. Board settings

Plug the board in with a data-capable USB-C cable. In the *Tools* menu:

| Setting | Value |
|---|---|
| Board | **ESP32C3 Dev Module** |
| Port | the new port that appeared (`COMx`, `/dev/cu.usbmodem...`, `/dev/ttyACM0`) |
| USB CDC On Boot | **Enabled**, otherwise you will see no output at all |
| CPU Frequency | 160 MHz (the default) |
| Everything else | leave the defaults |

The Super Mini has no USB-to-serial chip. The ESP32-C3 talks USB itself, on
pins 18 and 19. That is why *USB CDC On Boot* matters: it routes `Serial` to
that USB port.

On **Linux**, add yourself to the `dialout` group (`sudo usermod -aG dialout
$USER`, then log out and in) so you may open the port.

## 3. First upload

1. Open `lab1-time-latency/clocks/clocks.ino`.
2. Click *Upload* (the arrow). The first compile takes a few minutes.
3. Open *Tools > Serial Monitor* and set it to **115200 baud**.
4. Press the **RST** button on the board. You should see
   `=== lab 1: clocks ===` followed by the measurements.

To send a key to a sketch, type it in the serial monitor's input line and
press Enter. The sketches ignore the Enter itself.

## 4. When the upload fails or the port disappears

A sketch that crashes or keeps the CPU busy can take the USB port down with
it. The chip itself is fine. Put it in bootloader mode by hand:

1. Hold **BOOT**.
2. Press and release **RST** (or unplug and replug the cable).
3. Release **BOOT**.
4. Upload again, then press **RST** to start the sketch.

If *Upload* hangs at "Connecting...", do the same thing while it is waiting.

## 5. The board

| | |
|---|---|
| CPU | one RISC-V core, 160 MHz |
| RAM | 400 KB (about 320 KB free for sketches) |
| Flash | 4 MB, code runs from it through a 16 KB cache |
| Blue LED | GPIO 8, lit when the pin is LOW |
| BOOT button | GPIO 9, reads LOW when pressed |
| Free pins | 0-7, 10, 20, 21 (the labs need none of them) |

Pins 2, 8 and 9 are strapping pins: their level at reset decides how the chip
boots. Do not wire anything to them that pulls them hard in either direction.

## 6. Command line (optional)

Everything the IDE does is available from `arduino-cli`. The IDE ships a copy.

```
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc lab5-shared-resources/race
arduino-cli upload  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc -p /dev/ttyACM0 lab5-shared-resources/race
arduino-cli monitor -p /dev/ttyACM0 --config baudrate=115200
```

`arduino-cli monitor ... | tee run.log` also saves everything to a file, which
`tools/plot_serial.py` can plot (Lab 1).


===== FILE: common/rt.h =====

```cpp
// rt.h - the small toolbox shared by every lab sketch
//
// This is the master copy. tools/sync_common.sh copies it next to every .ino
// file, because the Arduino IDE only finds headers in the sketch folder. Edit
// it here, never in a lab folder.
//
// Read it once; every function is one idea:
//
//   rt_begin()         open USB serial, calibrate rt_busy_us(), print a banner
//   rt_cycles()        CPU cycle counter, 160 per microsecond (stops while the CPU sleeps)
//   rt_now_us()        microseconds since boot (esp_timer)
//   rt_sleep_until_tick()  absolute sleep, the basis of periodic tasks
//   rt_busy_us()       spend N microseconds of CPU time: the "computation"
//   rt_start_task()    xTaskCreate that stops the sketch if it fails
//   struct rt_stats    min / avg / max and a histogram of integer samples
//   rt_tl_...()        the timeline: which task had the CPU in each time slice
//   rt_read_key()      one key from the serial monitor, or -1
//   rt_verdict()       the one line a lab run is judged by
//
// The Linux lab track has an rt.h with the same names for the same ideas.
// The code is plain C in style: structs are passed by address (&s) and their
// fields reached with -> inside the helpers.
//
// Rules the labs follow:
//   - Nothing is printed inside a timing loop. Record, then report.
//   - Priorities used by lab tasks stay between 1 and 20. The esp_timer task
//     runs at 22 and the Wi-Fi task at 23; do not go above them.
//   - The ESP32-C3 has one core. Every task competes for the same CPU.

#pragma once

#include <Arduino.h>

#include "esp_cpu.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define CPU_MHZ 160          // the Arduino core runs the C3 at 160 MHz
#define LED_PIN 8            // blue LED on the Super Mini, lit when LOW
// BOOT_PIN (9) comes from the Arduino core: the BOOT button, LOW when pressed.

// ------------------------------------------------------------------ time ----

// The cycle counter is 32 bits wide and wraps every 26.8 s. The difference of
// two readings, computed in uint32_t, is still correct across one wrap.
// It counts only while the CPU runs: when no task is ready, the idle task puts
// the CPU to sleep until the next interrupt, and the counter stops. Use it for
// code that keeps the CPU busy; use rt_now_us() for anything that may sleep.
static inline uint32_t rt_cycles(void) { return esp_cpu_get_cycle_count(); }
static inline uint32_t rt_cycles_to_us(uint32_t c) { return c / CPU_MHZ; }
static inline int64_t rt_now_us(void) { return esp_timer_get_time(); }

// Sleep until the tick counter reaches `tick`. Returns at once if that moment
// has already passed. This is an absolute sleep: the wake-up time does not
// depend on when we call it, so a periodic task built on it does not drift.
//
// xTaskDelayUntil(&prev, n) means "wake me at prev + n", and it requires
// prev to be in the past. So prev is "now" and n is the distance to `tick`.
static inline void rt_sleep_until_tick(TickType_t tick) {
    TickType_t now = xTaskGetTickCount();
    if ((int32_t)(tick - now) <= 0) return;  // already passed
    TickType_t prev = now;
    xTaskDelayUntil(&prev, tick - now);
}

// --------------------------------------------------------- busy work ------

// rt_busy_us(n) runs a fixed number of loop iterations. Its length is CPU time,
// not wall-clock time: if the task is preempted halfway, the rest of the work
// still has to be done afterwards. That is how real computation behaves, and
// it is what makes the response times in labs 2, 3 and 5 come out right.
static uint32_t g_loops_per_ms = 0;

// An optimising compiler removes loops that compute nothing. Two things stop
// it here: noinline keeps rt_spin_loops() a real function call, and the empty
// asm statement counts as "an effect", so every iteration has to stay.
static void __attribute__((noinline)) rt_spin_loops(uint32_t n) {
    for (uint32_t i = 0; i < n; i++) __asm__ volatile("");
}

static inline void rt_busy_us(uint32_t us) {
    // Whole milliseconds first, then the rest, so the product cannot overflow.
    uint32_t ms = us / 1000;
    for (uint32_t i = 0; i < ms; i++) rt_spin_loops(g_loops_per_ms);
    rt_spin_loops((us % 1000) * g_loops_per_ms / 1000);
}

// Measure how many loop iterations fit in one millisecond. Interrupts are
// off during each try, so nothing else is counted. The fastest of five tries
// is the one nobody disturbed.
static inline void rt_calibrate_busy(void) {
    // portENTER_CRITICAL wants a lock variable. On this one-core chip the
    // lock does nothing extra: "critical" simply means interrupts are off.
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    const uint32_t kTrial = 20000;
    uint32_t best = 0xFFFFFFFFu;
    for (int t = 0; t < 5; t++) {
        portENTER_CRITICAL(&mux);
        uint32_t c0 = rt_cycles();
        rt_spin_loops(kTrial);
        uint32_t c = rt_cycles() - c0;
        portEXIT_CRITICAL(&mux);
        if (c < best) best = c;
    }
    // iterations per ms = kTrial / (best cycles / 160000 cycles per ms)
    g_loops_per_ms = (uint32_t)((uint64_t)kTrial * CPU_MHZ * 1000 / best);
}

// ------------------------------------------------------------- tasks ------

static inline void rt_fatal(const char* msg) {
    Serial.printf("\nFATAL: %s\n", msg);
    Serial.flush();
    for (;;) delay(1000);
}

// Create a task and give its handle back. Running out of memory is a bug in
// the lab setup, so it stops the sketch instead of carrying on without it.
// This one takes the stack size; rt_start_task() below is the usual case.
static inline TaskHandle_t rt_start_task_stack(TaskFunction_t fn, const char* name, UBaseType_t prio,
                                               void* arg, uint32_t stack) {
    TaskHandle_t h = NULL;
    if (xTaskCreate(fn, name, stack, arg, prio, &h) != pdPASS) rt_fatal("xTaskCreate failed");
    return h;
}

// The usual case: a 4096-byte stack. Pass NULL as arg if the task needs none.
static inline TaskHandle_t rt_start_task(TaskFunction_t fn, const char* name, UBaseType_t prio, void* arg) {
    return rt_start_task_stack(fn, name, prio, arg, 4096);
}

// ------------------------------------------------------------- stats ------

#define STATS_BUCKETS 12

struct rt_stats {
    uint32_t n;
    int32_t min, max;
    int64_t sum;
    int32_t bucket;              // width of one histogram bucket
    uint32_t hist[STATS_BUCKETS];  // the last bucket collects everything above
};

static inline void rt_stats_init(struct rt_stats* s, int32_t bucket) {
    memset(s, 0, sizeof(*s));
    s->min = INT32_MAX;
    s->max = INT32_MIN;
    s->bucket = bucket > 0 ? bucket : 1;
}

static inline void rt_stats_add(struct rt_stats* s, int32_t v) {
    s->n++;
    s->sum += v;
    if (v < s->min) s->min = v;
    if (v > s->max) s->max = v;
    int32_t b = v < 0 ? 0 : v / s->bucket;
    if (b >= STATS_BUCKETS) b = STATS_BUCKETS - 1;
    s->hist[b]++;
}

static inline int32_t rt_stats_avg(const struct rt_stats* s) { return s->n ? (int32_t)(s->sum / (int64_t)s->n) : 0; }

static inline void rt_stats_print(const struct rt_stats* s, const char* what, const char* unit) {
    if (s->n == 0) {
        Serial.printf("%s: no samples\n", what);
        return;
    }
    Serial.printf("%s over %lu samples: min %ld %s, avg %ld %s, max %ld %s\n", what,
                  (unsigned long)s->n, (long)s->min, unit, (long)rt_stats_avg(s), unit, (long)s->max, unit);
}

// One row per bucket, with a bar of '#' scaled to the fullest bucket. A bucket
// that is not empty gets at least one '#', so a single outlier stays visible.
#define STATS_BAR 40

static inline void rt_stats_print_hist(const struct rt_stats* s, const char* unit) {
    uint32_t top = 1;
    for (int b = 0; b < STATS_BUCKETS; b++)
        if (s->hist[b] > top) top = s->hist[b];

    for (int b = 0; b < STATS_BUCKETS; b++) {
        long lo = (long)b * s->bucket;
        char hi[12];
        if (b < STATS_BUCKETS - 1)
            snprintf(hi, sizeof(hi), "%ld", lo + s->bucket);
        else
            snprintf(hi, sizeof(hi), "and up");

        char bar[STATS_BAR + 1];
        int len = (int)((uint64_t)s->hist[b] * STATS_BAR / top);
        if (s->hist[b] > 0 && len == 0) len = 1;
        memset(bar, '#', len);
        bar[len] = '\0';

        Serial.printf("  %7ld .. %7s %-3s %7lu  %s\n", lo, hi, unit, (unsigned long)s->hist[b], bar);
    }
}

// ---------------------------------------------------------- timeline ------

// A strip with one character per slice of time. A task that is running writes
// its letter into the slice for "now"; slices nobody wrote stay '.'. Printing
// the strip afterwards shows who had the CPU and when.
#define TL_MAX 400

struct rt_timeline {
    int64_t t0;
    int32_t slice_us;
    int n;
    char c[TL_MAX];
};

static struct rt_timeline g_tl;

static inline void rt_tl_start(int32_t slice_us, int n) {
    g_tl.slice_us = slice_us;
    g_tl.n = n < TL_MAX ? n : TL_MAX;
    memset(g_tl.c, '.', sizeof(g_tl.c));
    g_tl.t0 = rt_now_us();
}

static inline void rt_tl_mark(char who) {
    int64_t i = (rt_now_us() - g_tl.t0) / g_tl.slice_us;
    if (i >= 0 && i < g_tl.n) g_tl.c[i] = who;
}

// rt_busy_us() that also marks the timeline every 100 us of work.
static inline void rt_tl_busy(uint32_t us, char who) {
    for (uint32_t done = 0; done < us; done += 100) {
        rt_tl_mark(who);
        rt_busy_us(us - done < 100 ? us - done : 100);
    }
    rt_tl_mark(who);
}

// Milliseconds since rt_tl_start(), for event logs printed next to the strip.
static inline float rt_tl_ms(void) { return (rt_now_us() - g_tl.t0) / 1000.0f; }

static inline void rt_tl_print(int per_row) {
    Serial.printf("timeline, one column = %ld us ('.' = nobody marked it)\n", (long)g_tl.slice_us);
    for (int row = 0; row < g_tl.n; row += per_row) {
        Serial.printf("%6ld ms |", (long)((int64_t)row * g_tl.slice_us / 1000));
        for (int i = row; i < row + per_row && i < g_tl.n; i++) Serial.write(g_tl.c[i]);
        Serial.println("|");
    }
}

// ------------------------------------------------------------ serial ------

static inline int rt_read_key(void) {
    if (!Serial.available()) return -1;
    int k = Serial.read();
    return (k == '\r' || k == '\n' || k == ' ') ? -1 : k;
}

static inline void rt_verdict(const char* v) { Serial.printf("VERDICT: %s\n", v); }

// Call first in setup(). Waits up to 3 s for the serial monitor so the first
// lines are not lost, then calibrates busy work at the highest lab priority.
static inline void rt_begin(const char* title) {
    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);

    UBaseType_t old = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 20);
    rt_calibrate_busy();
    vTaskPrioritySet(NULL, old);

    Serial.printf("\n=== %s ===\n", title);
    Serial.printf("ESP32-C3 at %d MHz, tick %d Hz, rt_busy_us: %lu loops per ms\n", CPU_MHZ,
                  configTICK_RATE_HZ, (unsigned long)g_loops_per_ms);
}
```


===== FILE: lab1-time-latency/README.md =====

# Lab 1 — Time, latency and jitter

Get the board running, see what the clocks on it can measure, and watch how
regular a periodic task really is: relative against absolute sleeps, the
effect of priorities, and the interference from a radio sharing the same CPU.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours, including 30 minutes of setup.

Theory behind this lab (concept pages, in Romanian):
[Ce înseamnă timp real](https://github.com/automatica-cluj/rt-concepts/blob/main/00-what-is-real-time.md),
[Ce este un sistem de operare de timp real](https://github.com/automatica-cluj/rt-concepts/blob/main/01-what-is-an-rtos.md),
[Ceasuri și rezoluție](https://github.com/automatica-cluj/rt-concepts/blob/main/1-time/clocks-and-resolution.md),
[Latență și jitter](https://github.com/automatica-cluj/rt-concepts/blob/main/1-time/latency-and-jitter.md),
[Eliberare periodică](https://github.com/automatica-cluj/rt-concepts/blob/main/1-time/periodic-release.md).

---

## 1. Setup

Follow [../docs/setup.md](../docs/setup.md) up to and including step 3. When
`clocks` prints its first lines, you are ready.

Every sketch includes a copy of [../common/rt.h](../common/rt.h). The
functions `rt_now_us()`, `rt_cycles()`, `rt_busy_us()`, `rt_start_task()` and
`rt_verdict()` that you will see in the sketches come from there. You do not
need to read it now.

## 2. Clocks: `clocks`

Upload `clocks/clocks.ino`, open the serial monitor, press RST. Press `r` to
run it again.

The sketch prints five parts:

1. **Resolution.** The smallest step each clock can show. `millis()` moves in
   whole milliseconds, `micros()` and `esp_timer_get_time()` in microseconds,
   and the cycle counter in 1/160 µs.
2. **Cost.** How many CPU cycles one reading takes (the average of 1000
   readings, with the cost of measuring subtracted). A clock that costs more
   to read than the thing you time is useless for that job.
3. **Sleeping.** `delay(1)` and `vTaskDelay(1)` do not mean "1000 µs".
   FreeRTOS counts delays in ticks of 1 ms, and "one tick from now" can be
   only a few microseconds away if you call it just before a tick. That is
   why the minimum is so small.
4. **Busy waiting.** `delayMicroseconds()` watches the clock, so it stops on
   time. `rt_busy_us()` runs a fixed number of loop iterations, counted at
   start-up with interrupts off. With the tick interrupt now taking a little
   CPU every millisecond, it comes out slightly longer than asked. That is
   intended: every later lab uses `rt_busy_us()` as "computation time", and
   real computation is stretched by interrupts in the same way.
5. **Printing.** The cost of one `Serial.printf`, next to the cost of reading
   a clock. This is why the labs never print inside a timing loop: they record
   into an array and print afterwards.

Write down, for your report:

| clock                  | resolution | cost (cycles) |
|------------------------|------------|---------------|
| `millis()`             |            |               |
| `micros()`             |            |               |
| `esp_timer_get_time()` |            |               |
| cycle counter          |            |               |

and the minimum and maximum of `delay(1)`.

## 3. A periodic task: `periodic_jitter`

Upload `periodic_jitter/periodic_jitter.ino`. A task wants to run every
**T = 10 ms** and computes **C = 3 ms** each time. It records its wake-up
time in every period. After 300 periods it prints:

- **interval** between consecutive wake-ups: ideally exactly 10 000 µs
- **lateness**: wake-up time minus the ideal time `start + k*T`. A perfect
  task has lateness 0 in every period. **Jitter** is max − min lateness.
- **drift**: how much longer 299 periods took than 299 × T
- a histogram of lateness, and a `VERDICT:` line

Keys `a` to `e` select the variant. Write down the `VERDICT:` line of each
one.

### Experiment A — relative sleep (key `a`)

Press `a`. The task does its 3 ms of work, then sleeps with `vTaskDelay(10)`.

**Look at** the interval and the drift line.

**What it shows.** The interval is about T + C, not T: the 10 ms sleep starts
only after the 3 ms of work. Every period adds the same error, so the task
falls further behind with every period. That growing error is drift.

### Experiment B — absolute sleep (key `b`)

Press `b`. The task now uses `xTaskDelayUntil(&last, 10)`: "wake me at
`last + 10`", counted from the previous wake-up time, not from *now*.

**Look at** the interval, the drift and the jitter.

**What it shows.** The drift is gone: the work no longer adds to the period.
The jitter is very small, possibly 0. The task is woken by the tick interrupt,
and with nothing else running, getting from the tick to the task takes the
same time in every period. This run is the baseline for C, D and E.

### Experiment C — a higher-priority task in the way (key `c`)

Press `c`. A second task, the "hog", runs at priority 10, above our task
(priority 5). It is released every 9 ms and computes 2 ms. Because 9 ms is not
a multiple of 10 ms, it is sometimes running just when our task should wake
up. The hog has a deadline of its own, 4 ms after each release, and the
report prints a `hog:` line with its response times and missed deadlines.

**Look at** the lateness histogram next to the one from B, and the `hog:`
line.

**What it shows.** Our task is now late in some periods, by up to about 2 ms,
the hog's computation time: when the hog is running, our task has to wait
until it finishes. The hog itself meets its deadline, because nothing
interrupts it.

### Experiment D — raise our priority (key `d`)

Press `d`. The same hog, but our task now has priority 15, above the hog.

**Look at** the jitter, then the `hog:` line next to the one from C.

**What it shows.** Our task is on time again. The hog now waits whenever our
task is running, so its response time can grow to about 2 ms + 3 ms = 5 ms,
past its 4 ms deadline, and some of its jobs miss. Raising one priority did
not remove the interference. It moved it to the other task.

### Experiment E — the radio (key `e`)

Press `e`. No hog. Instead the Wi-Fi radio scans for networks. The Wi-Fi
driver runs as a task with priority 23 and in interrupts, both above any task
you create. Our task keeps priority 15.

**Look at** the jitter next to B and D, and the line
`late by more than 100 us`. Run `e` two or three times.

**What it shows.** Only a few of the 300 periods are late, and the average
stays close to 0, but the worst case is clearly worse than in B. For a
control loop the worst case is what counts. Raising our priority cannot help,
because the radio runs above every priority we can choose. Boards with two
cores, such as the ESP32 or ESP32-S3, solve this by running the radio on one
core and the control loop on the other.

### Optional — plot it

The plot script needs Python 3 with matplotlib. Install it once, in a
virtual environment inside the repository:

```
python3 -m venv .venv
.venv/bin/pip install matplotlib
```

On Windows: `py -m venv .venv`, then `.venv\Scripts\pip install matplotlib`,
and use `.venv\Scripts\python` instead of `.venv/bin/python` below.

After a run, press `p` and save the serial output to a file (copy it from the
monitor, or use `arduino-cli monitor ... | tee run.log`, see setup.md). Then:

```
.venv/bin/python tools/plot_serial.py run.log          # opens a window
.venv/bin/python tools/plot_serial.py run.log b.png    # writes a PNG instead
```

The left plot shows the interval between wake-ups in every period, the right
one a histogram of those intervals (log scale, so a single outlier is
visible). One late wake-up shows as a long interval followed by a short one.
Plotting B, C and E side by side shows the three cases at a glance.

### Optional — watch it on a logic analyzer (keys `a` to `d`)

This part needs a logic analyzer, for example a Saleae with the Logic 2
application. The sketch drives two pins, and they only change during a run,
in the 3 s after you press a key:

| Analyzer | Board pin | High while |
|---|---|---|
| channel 0 | `0` (GPIO0) | the periodic task computes (its C = 3 ms) |
| channel 1 | `1` (GPIO1) | a hog job runs, from its release until it is done |
| ground | `G` | |

On a Saleae, the channel wires are coloured like resistor bands: channel 0
is **black**, channel 1 brown. The grey wires are ground. Black is not
ground: clipping it to `G` leaves channel 0 empty. One grey wire on `G` is
enough, since all grounds are connected inside the analyzer. Take care that
the clip on `G` does not touch `5V` or `3V3` next to it.

In Logic 2, in the Device Settings panel:

- enable digital channels 0 and 1 only
- set the digital sample rate to 10 MS/s (0.1 µs steps, well below
  anything this sketch does). Higher rates also record nanosecond spikes
  from the clips, and one such spike is enough to fire the trigger
- choose the **Trigger** capture mode: rising edge on channel 0, 5 s of
  capture after the trigger
- if the trigger still fires before you press a key, turn on the glitch
  filter for channels 0 and 1 at about 1 µs. The real pulses are
  milliseconds long, so the filter removes only spikes

Press Start. Logic 2 waits for the trigger. Then press the key in the
serial monitor: the capture starts at the first period of the run and stops
by itself. Press Start again before the next key. Hovering over a pulse
shows its width, and a pair of Timing Markers on two edges shows the time
between them.

**A (key `a`).** Only channel 0 moves.

**Look at** the width of a pulse, the time from one rising edge to the next,
and the time from the first rising edge of the run to the last.

**What it shows.** The pulse is C, 3 ms. The rising edges are T + C apart:
3 ms high, then the full 10 ms sleep low. The whole run takes longer than
299 × 10 ms by the drift the report printed. The drift is measured here on a
clock the board does not control.

**B (key `b`).** Only channel 0 moves.

**Look at** the time between rising edges, then zoom in on a few edges.

**What it shows.** The rising edges are 10 ms apart, the low part shrinks to
T − C, and the edges move by far less than the 1 ms tick. This is the
baseline.

**C (key `c`).** Both channels move: 2 ms pulses every 9 ms on channel 1,
3 ms pulses every 10 ms on channel 0. Because 9 ms and 10 ms differ, the two
slide past each other.

**Look at** a place where a channel 1 pulse is high at the moment channel 0
should rise.

**What it shows.** There, channel 0 rises only when channel 1 falls: our
task was ready, but the hog had the CPU. The distance from the 10 ms grid to
that edge is the lateness in the report, up to one hog job.

**D (key `d`).** Both channels move, as in C.

**Look at** the same kind of overlap.

**What it shows.** Channel 0 now rises on the grid every time. Instead, the
channel 1 pulse is 3 ms longer wherever the two overlap: it stays high while
channel 0 is high, because the hog job was preempted and finished later. The
interference moved from our task to the hog, which is what the `hog:` line in
D reports as longer response times and missed deadlines.

Key `e` drives only channel 0. The Wi-Fi activity that delays it is not on a
pin, so the analyzer shows the few late edges but not their cause.

**Cross-check.** The sketch measures itself with the board's own clock, and
the analyzer measures the same events from outside. Export the channel 0
edges (File, Export Data, CSV), compute
lateness = edge − (first edge + k × 10 ms) for each period, and compare
max − min with the jitter the sketch printed. The two agree closely: the pin
goes high a few instructions after the sketch takes its timestamp, and
`rt_now_us()` counts whole microseconds. The analyzer also shows something
the sketch cannot. Over the 3 s of a B run, the time from the first to the
last rising edge differs slightly from 299 × 10 ms, because the board's
crystal is not exactly its nominal frequency. The sketch cannot see this,
since it measures time with that same crystal. Strictly, the analyzer shows
the difference between two crystals, the board's and its own.

## Rules for these labs

- Every experiment ends with a `VERDICT:` line. Quote it in your report.
- If the board stops responding, see the recovery steps in setup.md. Nothing
  you do in these labs can damage it.


===== FILE: lab1-time-latency/clocks/clocks.ino =====

```cpp
// clocks - what the clocks on the ESP32-C3 can tell you, and what they cost
//
// Runs once after reset, and again on every 'r' in the serial monitor:
//   1. resolution: the smallest step each clock can show
//   2. cost: how long one reading takes, in CPU cycles
//   3. sleeps: how long delay(1), delay(5) and vTaskDelay(1) really last
//   4. busy waits: delayMicroseconds() and rt_busy_us() against the clock
//   5. printing: how long one Serial.printf blocks the caller
//
// Everything is measured first and printed afterwards, except part 5, which
// measures printing on purpose.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const int kSleepSamples = 20;
const int kPrintSamples = 20;

// Smallest non-zero step of a clock, found by reading it until it changes.
static uint32_t step_millis(void) {
    uint32_t a = millis();
    uint32_t b = millis();
    while (b == a) b = millis();  // wait for a step, so we start right at one
    a = b;
    while (b == a) b = millis();  // read until the next step
    return b - a;
}

static uint32_t step_micros(void) {
    uint32_t a = micros();
    uint32_t b = micros();
    while (b == a) b = micros();  // wait for a step, so we start right at one
    a = b;
    while (b == a) b = micros();  // read until the next step
    return b - a;
}

static int64_t step_esp_timer(void) {
    int64_t a = rt_now_us();
    int64_t b = rt_now_us();
    while (b == a) b = rt_now_us();  // wait for a step, so we start right at one
    a = b;
    while (b == a) b = rt_now_us();  // read until the next step
    return b - a;
}

static void part_resolution(void) {
    Serial.println("\n1. resolution (smallest step you can see)");
    Serial.printf("   millis()             %lu ms\n", (unsigned long)step_millis());
    Serial.printf("   micros()             %lu us\n", (unsigned long)step_micros());
    Serial.printf("   esp_timer_get_time() %lld us\n", (long long)step_esp_timer());
    uint32_t a = rt_cycles(), b = rt_cycles();
    Serial.printf("   cycle counter        %lu cycle(s) between two reads = %.4f us\n",
                  (unsigned long)(b - a), (b - a) / (float)CPU_MHZ);
}

// Cycles per reading: the time between two cycle-counter reads with the clock
// read in between, minus the same pair with nothing in between.
static void print_cost(const char* what, uint32_t cycles, uint32_t empty_cycles, int n) {
    int32_t per = ((int32_t)cycles - (int32_t)empty_cycles) / n;
    Serial.printf("   %-20s %4ld cycles = %.2f us\n", what, (long)per, per / (float)CPU_MHZ);
}

static void part_cost(void) {
    const int kN = 1000;
    volatile uint64_t sink = 0;  // stops the compiler from dropping the calls
    uint32_t c0, c_empty = 0, c_millis = 0, c_micros = 0, c_timer = 0, c_cycles = 0;

    for (int i = 0; i < kN; i++) {
        c0 = rt_cycles();
        sink = 0;  // no clock: what the measuring itself costs
        c_empty += rt_cycles() - c0;

        c0 = rt_cycles();
        sink = millis();
        c_millis += rt_cycles() - c0;

        c0 = rt_cycles();
        sink = micros();
        c_micros += rt_cycles() - c0;

        c0 = rt_cycles();
        sink = rt_now_us();
        c_timer += rt_cycles() - c0;

        c0 = rt_cycles();
        sink = rt_cycles();
        c_cycles += rt_cycles() - c0;
    }
    (void)sink;  // written only, on purpose

    Serial.printf("\n2. cost of one reading (average of %d, measuring overhead of %lu cycles taken off)\n", kN,
                  (unsigned long)(c_empty / kN));
    print_cost("millis()", c_millis, c_empty, kN);
    print_cost("micros()", c_micros, c_empty, kN);
    print_cost("esp_timer_get_time()", c_timer, c_empty, kN);
    print_cost("cycle counter", c_cycles, c_empty, kN);
}

// How long does "sleep for N ms" last? The start is a random point between
// two ticks (we burn a random amount first), as it would be in real code.
static void measure_sleep(const char* what, int kind, int ms) {
    struct rt_stats s;
    rt_stats_init(&s, 250);
    for (int i = 0; i < kSleepSamples; i++) {
        rt_busy_us(random(0, 1000));
        int64_t t0 = rt_now_us();
        if (kind == 0)
            delay(ms);
        else
            vTaskDelay(ms);  // in ticks; 1 tick = 1 ms here
        rt_stats_add(&s, (int32_t)(rt_now_us() - t0));
    }
    Serial.printf("   %-14s asked %5d us: min %5ld, avg %5ld, max %5ld us\n", what, ms * 1000, (long)s.min,
                  (long)rt_stats_avg(&s), (long)s.max);
}

static void part_sleep(void) {
    Serial.printf("\n3. sleeping (%d tries each, started at random points in a tick)\n", kSleepSamples);
    measure_sleep("delay(1)", 0, 1);
    measure_sleep("delay(5)", 0, 5);
    measure_sleep("vTaskDelay(1)", 1, 1);
    measure_sleep("vTaskDelay(5)", 1, 5);
}

static void part_busy(void) {
    Serial.println("\n4. busy waiting (the task keeps the CPU the whole time)");
    const uint32_t kUs[] = {10, 100, 1000, 5000};
    for (int i = 0; i < 4; i++) {
        int64_t t0 = rt_now_us();
        delayMicroseconds(kUs[i]);
        int64_t t1 = rt_now_us();
        rt_busy_us(kUs[i]);
        int64_t t2 = rt_now_us();
        Serial.printf("   %5lu us: delayMicroseconds %6lld us, rt_busy_us %6lld us\n", (unsigned long)kUs[i],
                      (long long)(t1 - t0), (long long)(t2 - t1));
    }
}

static void part_print(void) {
    Serial.printf("\n5. cost of Serial.printf of a 40-character line (%d tries)\n", kPrintSamples);
    int32_t us[kPrintSamples];
    for (int i = 0; i < kPrintSamples; i++) {
        int64_t t0 = rt_now_us();
        Serial.printf("   line %2d: the quick brown fox jumps.\n", i);
        us[i] = (int32_t)(rt_now_us() - t0);
    }
    struct rt_stats s;
    rt_stats_init(&s, 100);
    for (int i = 0; i < kPrintSamples; i++) rt_stats_add(&s, us[i]);
    rt_stats_print(&s, "   printf", "us");
}

static void run_all(void) {
    part_resolution();
    part_cost();
    part_sleep();
    part_busy();
    part_print();
    rt_verdict("CLOCKS_DONE");
    Serial.println("\npress r to run again");
}

void setup() {
    rt_begin("lab 1: clocks");
    run_all();
}

void loop() {
    if (rt_read_key() == 'r') run_all();
    delay(20);
}
```


===== FILE: lab1-time-latency/periodic_jitter/periodic_jitter.ino =====

```cpp
// periodic_jitter - how regular is a periodic task?
//
// One task wants to run every 10 ms and computes for 3 ms each time. It records
// the moment it woke up in every period and prints the numbers when it is done.
// Keys in the serial monitor pick the variant:
//
//   a  relative sleep: vTaskDelay(10) after the work           (priority 5)
//   b  absolute sleep: xTaskDelayUntil(&last, 10)              (priority 5)
//   c  like b, plus a "hog" task at priority 10, released every
//      9 ms, that computes 2 ms and must be done 4 ms after its
//      release                                                 (priority 5)
//   d  like c, but the periodic task now has priority 15
//   e  like d, but instead of the hog the Wi-Fi radio scans for networks
//   p  print the samples of the last run as CSV (for tools/plot_serial.py)
//
// "lateness" of period k = wake-up time - (start + k * period). A perfect task
// has lateness 0 in every period. jitter = max lateness - min lateness.
//
// For a logic analyzer: GPIO0 is high while the periodic task computes, GPIO1
// while a hog job runs (from its release until it is done). Connect the
// analyzer's ground to GND. The pins are set with gpio_set_level(), which takes
// well under a microsecond, so they do not change the timing.

#include <WiFi.h>

#include "driver/gpio.h"
#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const int kPeriodMs = 10;      // T
const int kWorkUs = 3000;      // C, burned with rt_busy_us()
const int kPeriods = 300;      // 3 s per run
const int kHogPeriodMs = 9;        // the hog is released this often ...
const int kHogWorkUs = 2000;       // ... computes this long ...
const int kHogDeadlineUs = 4000;   // ... and must be done this long after its release
const UBaseType_t kHogPrio = 10;
const int kLateUs = 100;           // a period later than this counts as "late"
const int kPinTask = 0;            // logic analyzer: periodic task computing
const int kPinHog = 1;             // logic analyzer: hog job running

static int64_t g_wake[kPeriods];
static bool g_absolute;
static volatile bool g_stop;
static SemaphoreHandle_t g_done;  // every task gives it once when it ends
static int g_ran = 0;             // samples in g_wake from the last run
static bool g_hog_on;             // the hog ran in the last run
static struct rt_stats g_hog;     // response time of each hog job
static int g_hog_misses;          // jobs that finished after kHogDeadlineUs

static void periodic_task(void* arg) {
    (void)arg;  // not used
    TickType_t last = xTaskGetTickCount();
    for (int k = 0; k < kPeriods; k++) {
        if (g_absolute)
            xTaskDelayUntil(&last, pdMS_TO_TICKS(kPeriodMs));  // wake at last + T
        else
            vTaskDelay(pdMS_TO_TICKS(kPeriodMs));  // wake T after *now*
        g_wake[k] = rt_now_us();
        gpio_set_level((gpio_num_t)kPinTask, 1);
        rt_busy_us(kWorkUs);
        gpio_set_level((gpio_num_t)kPinTask, 0);
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void hog_task(void* arg) {
    (void)arg;  // not used
    vTaskDelay(1);  // start just after a tick, so the tick count and the clock line up
    TickType_t last = xTaskGetTickCount();
    int64_t release = rt_now_us();  // the release at tick 'last', in microseconds
    while (!g_stop) {
        gpio_set_level((gpio_num_t)kPinHog, 1);
        rt_busy_us(kHogWorkUs);
        gpio_set_level((gpio_num_t)kPinHog, 0);
        // Response time: from the release, not from the moment the hog got the
        // CPU, so time spent waiting for a higher priority task counts too.
        int32_t response = (int32_t)(rt_now_us() - release);
        rt_stats_add(&g_hog, response);
        if (response > kHogDeadlineUs) g_hog_misses++;
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kHogPeriodMs));  // next release: last + 9 ms
        release += kHogPeriodMs * 1000;
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// Scanning makes the radio hop channels and send probe requests. The Wi-Fi
// driver does that work in its own task at priority 23 and in interrupts.
static void wifi_task(void* arg) {
    (void)arg;  // not used
    while (!g_stop) WiFi.scanNetworks(false);
    WiFi.mode(WIFI_OFF);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// Everything in report() is arithmetic on the recorded wake-up times and
// printing. It runs after the measurement, so it cannot disturb it.
static void report(void) {
    const int32_t T = kPeriodMs * 1000;

    // The ideal grid starts at the earliest possible origin, so the least
    // late period has lateness 0.
    int64_t origin = g_wake[0];
    for (int k = 1; k < kPeriods; k++)
        if (g_wake[k] - (int64_t)k * T < origin) origin = g_wake[k] - (int64_t)k * T;

    struct rt_stats interval, late;
    rt_stats_init(&interval, 1000);
    rt_stats_init(&late, 200);  // 12 buckets: 0 .. 2400 us, room for the hog's 2 ms
    int n_late = 0;
    for (int k = 0; k < kPeriods; k++) {
        int32_t l = (int32_t)(g_wake[k] - origin - (int64_t)k * T);
        rt_stats_add(&late, l);
        if (l > kLateUs) n_late++;
        if (k > 0) rt_stats_add(&interval, (int32_t)(g_wake[k] - g_wake[k - 1]));
    }

    rt_stats_print(&interval, "interval between wake-ups", "us");
    rt_stats_print(&late, "lateness", "us");
    Serial.printf("late by more than %d us: %d of %d periods\n", kLateUs, n_late, kPeriods);
    Serial.println("lateness histogram:");
    rt_stats_print_hist(&late, "us");

    int64_t took = g_wake[kPeriods - 1] - g_wake[0];
    int64_t should = (int64_t)(kPeriods - 1) * T;
    Serial.printf("drift: %d periods should take %.1f ms, took %.1f ms (%+.1f ms)\n", kPeriods - 1,
                  should / 1000.0, took / 1000.0, (took - should) / 1000.0);

    if (g_hog_on) {
        Serial.printf("hog: %lu jobs of %d us, response min %ld, avg %ld, max %ld us, %d missed the %d us deadline\n",
                      (unsigned long)g_hog.n, kHogWorkUs, (long)g_hog.min, (long)rt_stats_avg(&g_hog),
                      (long)g_hog.max, g_hog_misses, kHogDeadlineUs);
    }

    char v[48];
    if (took - should > T)
        snprintf(v, sizeof(v), "DRIFT_MS=%ld", (long)((took - should) / 1000));
    else
        snprintf(v, sizeof(v), "JITTER_US=%ld", (long)(late.max - late.min));
    rt_verdict(v);
}

static void run(char key) {
    UBaseType_t prio = (key == 'd' || key == 'e') ? 15 : 5;
    bool hog = (key == 'c' || key == 'd');
    bool wifi = (key == 'e');
    g_absolute = (key != 'a');

    const char* other = "nothing else running";
    if (hog) other = "hog at priority 10";
    if (wifi) other = "Wi-Fi scanning";
    Serial.printf("\n--- variant %c: %s sleep, periodic task priority %u, %s ---\n", key,
                  g_absolute ? "absolute" : "relative", (unsigned)prio, other);
    Serial.printf("period %d ms, work %d us, %d periods\n", kPeriodMs, kWorkUs, kPeriods);

    g_stop = false;
    g_hog_on = hog;
    rt_stats_init(&g_hog, 500);
    g_hog_misses = 0;
    int helpers = 0;
    if (hog) {
        rt_start_task(hog_task, "hog", kHogPrio, NULL);
        helpers++;
    }
    if (wifi) {
        WiFi.mode(WIFI_STA);
        rt_start_task_stack(wifi_task, "wifi_load", 1, NULL, 8192);
        helpers++;
        delay(500);  // let the radio start before measuring
    }

    rt_start_task(periodic_task, "periodic", prio, NULL);
    xSemaphoreTake(g_done, portMAX_DELAY);  // the periodic task is done
    g_stop = true;
    for (int i = 0; i < helpers; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    g_ran = kPeriods;

    report();  // printing starts only now that nothing is being measured
}

static void dump_csv(void) {
    if (g_ran == 0) {
        Serial.println("no run yet");
        return;
    }
    Serial.println("csv: k,wake_us,interval_us");
    for (int k = 0; k < g_ran; k++)
        Serial.printf("%d,%lld,%lld\n", k, (long long)(g_wake[k] - g_wake[0]),
                      (long long)(k ? g_wake[k] - g_wake[k - 1] : 0));
    Serial.println("csv: end");
}

static void menu(void) {
    Serial.println("\nkeys: a relative | b absolute | c + hog | d + hog, high priority | e + Wi-Fi | p CSV");
}

void setup() {
    rt_begin("lab 1: periodic_jitter");
    gpio_reset_pin((gpio_num_t)kPinTask);
    gpio_set_direction((gpio_num_t)kPinTask, GPIO_MODE_OUTPUT);
    gpio_reset_pin((gpio_num_t)kPinHog);
    gpio_set_direction((gpio_num_t)kPinHog, GPIO_MODE_OUTPUT);
    g_done = xSemaphoreCreateCounting(4, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'e') {
        run((char)k);
        menu();
    } else if (k == 'p') {
        dump_csv();
    }
    delay(20);
}
```


===== FILE: lab2-scheduling/README.md =====

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


===== FILE: lab2-scheduling/scheduling/scheduling.ino =====

```cpp
// scheduling - watch the FreeRTOS scheduler decide who gets the one CPU
//
// Worker tasks burn CPU time and mark a timeline while they run. The control
// code (loop(), priority 10 while it releases workers) creates them at known
// moments, then waits and prints the timeline. Keys pick the scenario:
//
//   a  preemption: L (prio 2) at 0 ms, M (prio 3) at 40 ms, H (prio 4) at 80 ms,
//      each computes 100 ms
//   b  equal priorities: A, B, C all at priority 3, released together,
//      40 ms each; the tick (1 ms) rotates them
//   c  a periodic task: H (prio 4) computes 4 ms every 20 ms, while
//      L (prio 2) computes 120 ms in one go
//   d  like a, and a snapshot of every task in the system taken at 60 ms
//
// In the timeline strip each column is a slice of time and the letter is the
// task that was running in it.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const UBaseType_t kControlPrio = 10;  // above every worker
const int kSnapshotMs = 60;           // when scenario d takes its snapshot

struct Worker {
    char letter;
    UBaseType_t prio;
    uint32_t work_us;   // total computation
    int jobs;           // 1 = one-shot; more = periodic
    int period_ms;      // for periodic workers
    float release_ms, start_ms, finish_ms;  // measured, from rt_tl_start()
};

static struct Worker g_w[3];
static int g_nw;
static SemaphoreHandle_t g_done;
static char g_snapshot[1200];

static void worker_task(void* arg) {
    struct Worker* w = (struct Worker*)arg;
    w->start_ms = rt_tl_ms();
    TickType_t last = xTaskGetTickCount();
    for (int j = 0; j < w->jobs; j++) {
        if (j > 0) xTaskDelayUntil(&last, pdMS_TO_TICKS(w->period_ms));
        rt_tl_busy(w->work_us, w->letter);
    }
    w->finish_ms = rt_tl_ms();
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void add_worker(char letter, UBaseType_t prio, uint32_t work_ms, int jobs, int period_ms) {
    struct Worker* w = &g_w[g_nw++];
    w->letter = letter;
    w->prio = prio;
    w->work_us = work_ms * 1000;
    w->jobs = jobs;
    w->period_ms = period_ms;
}

static void release(struct Worker* w) {
    char name[2] = {w->letter, 0};
    w->release_ms = rt_tl_ms();
    rt_start_task(worker_task, name, w->prio, w);
}

// Sleep until `ms` after rt_tl_start(). The control code has the highest
// priority, so it wakes on time and releases the next worker on time.
static void wait_until_ms(int ms) {
    int32_t left = ms - (int32_t)rt_tl_ms();
    if (left > 0) vTaskDelay(pdMS_TO_TICKS(left));
}

// The verdict: the workers' letters, earliest finish first, e.g. "HML".
// Only reporting: pick the earliest finisher not yet listed, g_nw times.
static void finish_order(char* order) {
    bool listed[3] = {false, false, false};
    for (int n = 0; n < g_nw; n++) {
        int best = -1;
        for (int i = 0; i < g_nw; i++) {
            if (listed[i]) continue;
            if (best < 0 || g_w[i].finish_ms < g_w[best].finish_ms) best = i;
        }
        listed[best] = true;
        order[n] = g_w[best].letter;
    }
    order[g_nw] = 0;  // end of string
}

static void run(char key) {
    g_nw = 0;
    int slice_us = 5000, columns = 70;
    int release_at[3] = {0, 0, 0};

    if (key == 'a' || key == 'd') {
        add_worker('L', 2, 100, 1, 0);
        add_worker('M', 3, 100, 1, 0);
        add_worker('H', 4, 100, 1, 0);
        release_at[1] = 40;
        release_at[2] = 80;
    } else if (key == 'b') {
        add_worker('A', 3, 40, 1, 0);
        add_worker('B', 3, 40, 1, 0);
        add_worker('C', 3, 40, 1, 0);
        slice_us = 1000;
        columns = 130;
    } else {  // 'c'
        add_worker('L', 2, 120, 1, 0);
        add_worker('H', 4, 4, 8, 20);
        slice_us = 1000;
        columns = 170;
    }

    Serial.printf("\n--- scenario %c ---\n", key);
    for (int i = 0; i < g_nw; i++)
        Serial.printf("%c: priority %u, released at %3d ms, %d job(s) of %lu ms%s\n", g_w[i].letter,
                      (unsigned)g_w[i].prio, release_at[i], g_w[i].jobs, (unsigned long)(g_w[i].work_us / 1000),
                      g_w[i].jobs > 1 ? ", periodic" : "");

    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, kControlPrio);
    rt_tl_start(slice_us, columns);
    bool snapshot_taken = false;
    for (int i = 0; i < g_nw; i++) {
        // The snapshot goes between the releases, so it is taken at
        // kSnapshotMs and not after the last release.
        if (key == 'd' && !snapshot_taken && release_at[i] > kSnapshotMs) {
            wait_until_ms(kSnapshotMs);
            vTaskList(g_snapshot);  // name, state, priority, least free stack (bytes), number, core
            snapshot_taken = true;
        }
        wait_until_ms(release_at[i]);
        release(&g_w[i]);
    }
    for (int i = 0; i < g_nw; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskPrioritySet(NULL, my_prio);

    rt_tl_print(100);
    Serial.println("task  prio  release   start  finish  response (ms)");
    for (int i = 0; i < g_nw; i++) {
        struct Worker* w = &g_w[i];
        if (w->jobs > 1)  // periodic: each job has its own response time, read it from the strip
            Serial.printf("  %c   %4u  %7.1f %7.1f %7.1f         -  (%d jobs, see the strip)\n", w->letter,
                          (unsigned)w->prio, w->release_ms, w->start_ms, w->finish_ms, w->jobs);
        else
            Serial.printf("  %c   %4u  %7.1f %7.1f %7.1f  %8.1f\n", w->letter, (unsigned)w->prio, w->release_ms,
                          w->start_ms, w->finish_ms, w->finish_ms - w->release_ms);
    }
    if (key == 'd') {
        Serial.printf("\nsnapshot at %d ms (X running, R ready, B blocked, S suspended, D deleted)\n", kSnapshotMs);
        Serial.println("name          state prio stack  #  core");
        Serial.print(g_snapshot);
    }

    char order[4];
    finish_order(order);
    char v[32];
    snprintf(v, sizeof(v), "FINISH_ORDER=%s", order);
    rt_verdict(v);
}

static void menu(void) {
    Serial.println("\nkeys: a preemption | b equal priorities | c periodic + long task | d a + task snapshot");
}

void setup() {
    rt_begin("lab 2: scheduling");
    g_done = xSemaphoreCreateCounting(3, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'd') {
        run((char)k);
        menu();
    }
    delay(20);
}
```


===== FILE: lab2-scheduling/starvation_wdt/starvation_wdt.ino =====

```cpp
// starvation_wdt - a task that never lets go of the CPU, and the watchdog
//
// loop() runs at priority 1. It blinks the LED and measures the longest gap
// between two of its own iterations. The task watchdog (TWDT) is set to 3 s
// and watches loop(): if loop() does not come round for 3 s, the chip resets.
//
// Keys start a "hog" task at priority 5 that computes without blocking:
//
//   a  hog computes 1 s, then ends          loop() starves for 1 s, no reset
//   b  hog computes 6 s                     loop() starves, TWDT resets the chip
//   c  hog computes 6 s, but sleeps 1 ms    loop() gets the gaps, no reset
//      after every 10 ms of work
//   d  like b, but the hog registers itself with the TWDT and feeds it
//      while it computes
//   i  print the reset report again (USB reconnects after a reset, so the
//      first lines are easy to miss)
//
// After a reset the sketch prints why the chip restarted and which variant was
// running at the time. That survives the reset in RTC memory.

#include "esp_task_wdt.h"
#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kWdtTimeoutMs = 3000;
const UBaseType_t kHogPrio = 5;

// Ordinary variables are cleared by a reset. These two live in RTC memory,
// which keeps its contents across a software or watchdog reset. That is how
// the sketch knows, after the reset, which variant was running. g_marker holds
// kMagic while a variant runs, to tell a real note from random bits.
const uint32_t kMagic = 0x5741544Bu;
RTC_NOINIT_ATTR uint32_t g_marker;
RTC_NOINIT_ATTR char g_variant;

static SemaphoreHandle_t g_done;
static volatile char g_running = 0;
static int64_t g_last_loop_us = 0;
static int64_t g_max_gap_us = 0;

static const char* reason_name(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON: return "POWERON (power applied)";
        case ESP_RST_SW: return "SW (esp_restart)";
        case ESP_RST_PANIC: return "PANIC (exception or abort)";
        case ESP_RST_INT_WDT: return "INT_WDT (interrupt watchdog)";
        case ESP_RST_TASK_WDT: return "TASK_WDT (task watchdog)";
        case ESP_RST_WDT: return "WDT (other watchdog)";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_USB: return "USB (reset by the USB host)";
        default: return "other";
    }
}

static void hog_task(void* arg) {
    (void)arg;  // not used
    char v = g_running;  // the variant, set by start_hog()
    uint32_t total_ms = (v == 'a') ? 1000 : 6000;
    if (v == 'd') esp_task_wdt_add(NULL);  // "I am alive, watch me"

    for (uint32_t ms = 0; ms < total_ms; ms += 10) {
        rt_busy_us(10000);
        if (v == 'c') vTaskDelay(1);         // give the CPU away for one tick
        if (v == 'd') esp_task_wdt_reset();  // feed the watchdog
    }

    if (v == 'd') esp_task_wdt_delete(NULL);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void start_hog(char v) {
    Serial.printf("\n--- variant %c: hog at priority %u for %s ---\n", v, (unsigned)kHogPrio,
                  v == 'a' ? "1 s" : "6 s");
    if (v == 'b' || v == 'd')
        Serial.printf("if the watchdog works, the board resets after about %lu ms\n", (unsigned long)kWdtTimeoutMs);
    Serial.flush();  // make sure the text is out before the CPU is taken away

    g_marker = kMagic;  // remembered across a reset
    g_variant = v;
    g_running = v;
    g_max_gap_us = 0;
    g_last_loop_us = rt_now_us();
    rt_start_task(hog_task, "hog", kHogPrio, NULL);
}

static void menu(void) {
    Serial.println("\nkeys: a hog 1 s | b hog 6 s | c hog 6 s with sleeps | d hog 6 s feeding the TWDT | i reset report");
}

// What caused the last reset, and which variant was running then.
static esp_reset_reason_t g_reason;
static char g_reset_during = 0;

static void reset_report(void) {
    Serial.printf("reset reason: %s\n", reason_name(g_reason));
    if (g_reset_during) {
        Serial.printf("the reset happened while variant %c was running\n", g_reset_during);
        char v[64];
        snprintf(v, sizeof(v), "RESET_DURING_%c reason=%s", g_reset_during,
                 g_reason == ESP_RST_TASK_WDT ? "TASK_WDT" : "OTHER");
        rt_verdict(v);
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    rt_begin("lab 2: starvation_wdt");

    // After power-on, RTC memory holds random bits, so the marker means
    // nothing.
    g_reason = esp_reset_reason();
    if (g_reason != ESP_RST_POWERON && g_marker == kMagic) g_reset_during = g_variant;
    g_marker = 0;
    reset_report();

    // The TWDT is already running (5 s, idle task not watched). Shorten it and
    // make it watch the task that runs loop().
    esp_task_wdt_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.timeout_ms = kWdtTimeoutMs;
    cfg.idle_core_mask = 0;
    cfg.trigger_panic = true;  // reset the chip instead of only printing
    if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) rt_fatal("esp_task_wdt_reconfigure failed");
    enableLoopWDT();  // the Arduino core now feeds the TWDT after every loop()

    g_done = xSemaphoreCreateBinary();
    menu();
}

void loop() {
    int64_t t = rt_now_us();
    if (g_running && t - g_last_loop_us > g_max_gap_us) g_max_gap_us = t - g_last_loop_us;
    g_last_loop_us = t;

    digitalWrite(LED_PIN, (millis() / 250) % 2);  // heartbeat: blinks while loop() runs

    if (g_running && xSemaphoreTake(g_done, 0) == pdTRUE) {
        Serial.printf("hog finished; longest gap between two loop() runs: %.1f ms\n", g_max_gap_us / 1000.0);
        char v[48];
        snprintf(v, sizeof(v), "NO_RESET max_loop_gap_ms=%ld", (long)(g_max_gap_us / 1000));
        rt_verdict(v);
        g_running = 0;
        g_marker = 0;
        menu();
    }

    int k = rt_read_key();
    if (!g_running && k >= 'a' && k <= 'd') start_hog((char)k);
    if (!g_running && k == 'i') reset_report();
    delay(5);
}
```


===== FILE: lab3-periodic-rm/README.md =====

# Lab 3 — Periodic tasks, WCET and Rate Monotonic scheduling

Most real-time software is a set of periodic tasks: read a sensor every 10 ms,
run a controller every 15 ms, log every 35 ms. To know in advance that every
deadline will be met you need two things: the worst-case execution time
(WCET) of each task, and a schedulability test. In this lab you see how the
WCET is measured, and how the tests compare with what the board actually
does.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 to 1.5 hours.

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

Experiments A, B and D sort the **same** input in every run: 64 random
numbers, made once at start-up (with a fixed seed, so every board gets the
same ones). The input therefore cannot be what makes one run slower than
another. Experiment C is about the input.

Right after reset the sketch prints the time of the **first** run and the
**second** run of the job, on that same input. The first is slower: its code
is not in the cache yet. Experiment D comes back to this.

### Experiment A — 1000 runs, interrupts on (key `a`)

Press `a`.

**Look at** min, avg and max, and the histogram. It shows how many cycles
each run took above the fastest run (160 cycles = 1 µs).

**What it shows.** Almost all runs take the same time. A few are clearly
slower, and they make the max. Those are the runs during which an interrupt
arrived, mostly the 1 ms tick: the CPU left the job, handled the interrupt
and came back, and the cycle counter kept counting. The shorter the job
compared with 1 ms, the fewer runs are hit.

### Experiment B — interrupts off (key `b`)

Press `b`. Each run is now measured with interrupts off
(`portENTER_CRITICAL`).

**Look at** the max next to the one from A.

**What it shows.** The slow runs are gone and max is close to avg. This is
the time of the job itself. A's max was the job plus an interrupt. A
schedulability analysis keeps the two apart: the job's own worst case is its
C, and the interrupts are counted separately as interference.

### Experiment C — input matters (key `c`)

Press `c`. The same job on already-sorted, random and reverse-sorted input.
Here "random" is a new random input in every run.

**Look at** the three rows.

**What it shows.** Sorted input is the fastest and reverse-sorted input the
slowest. Insertion sort moves every element past all the ones before it when
the input is reversed, the most work it can do. The random row spreads out
between the two, but random inputs almost never happen to be fully reversed,
so random testing would not find the worst case. The worst input has to be
worked out from how the code behaves.

### Experiment D — a cold cache (key `d`)

Press `d`. Before every run, the sketch reads a 64 KB table from flash. That
pushes the job's code out of the 16 KB cache, so it has to come from flash
again. (The cache only holds what lives in flash: code and constants. The
arrays the job sorts are in RAM and are not cached.)

**Look at** the times next to B and to the "first run" printed at reset.

**What it shows.** Every run is now slower than in B, and similar to the first
run after reset: each one starts with a cold cache. So the WCET of a piece of
code is not a property of the code alone. In A–D it changed with interrupts,
with the input, and with the state of the cache.

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
  stops changing. In words: a task's own computation, plus every job of a
  higher-priority task released while it is still waiting or running. If
  R ≤ D, the task always meets its deadline. RTA is exact for this model.
- What the board measured: max and average R, jobs, misses and skipped
  releases.

The task sets are tables at the top of the sketch:

| Key | Set (T, C) in ms | U | Priorities |
|---|---|---|---|
| `a` | sensor (10, 2), control (20, 4), logger (40, 8) | 0.60 | RM |
| `b` | sensor (10, 3), control (15, 4), logger (35, 10) | 0.85 | RM |
| `c` | sensor (10, 3), control (15, 5), logger (35, 14) | 1.03 | RM, late jobs skipped |
| `d` | same as b | 0.85 | reversed: logger highest |
| `e` | same as c | 1.03 | RM, late jobs run late |

### Experiment E — below the bound, above the bound (keys `a`, `b`)

Press `a`, then `b`.

**Look at** the line with U and the bound, the RTA column, and the measured
R max and misses.

**What it shows.** Set a is below the bound, so it is schedulable, and there
are no misses. Set b is above the bound, so the Liu & Layland test cannot
tell. RTA gives every task an R below its deadline, and indeed the board
misses nothing. The bound is sufficient, not necessary: passing it guarantees
the deadlines, and failing it does not mean a miss. The measured R max comes
out a little above RTA, because RTA does not count the tick interrupt and
the task switches.

### Experiment F — overload (key `c`)

Press `c`. U > 1, so something must miss.

**Look at** the `miss` column.

**What it shows.** Only the logger misses. The sensor and control are not
affected at all. With fixed priorities, an overload hurts the
lowest-priority tasks first, and the high-priority ones keep their
deadlines.

### Experiment G — the wrong priorities (key `d`)

Press `d`: set b again, U = 0.85, but the logger (the longest period) now has
the highest priority.

**Look at** the `miss` column, next to set b in experiment E.

**What it shows.** The same task set that met every deadline now misses. The
sensor, the task with the shortest period, misses: its 10 ms period cannot
fit behind the logger's 10 ms and the control's 4 ms. Priorities chosen by
"importance" instead of by period can break a task set that is schedulable
with RM.

### Experiment H — overrun policy (key `e`)

Press `e`: the overload of c, but a job that is late is still run, as soon as
possible (catch-up), instead of being skipped. `xTaskDelayUntil` does exactly
this by default when the wake-up time has already passed.

**Look at** the logger's R max and skipped count, next to c.

**What it shows.** No job is skipped, but the logger's response time is much
larger than in c. With U > 1 there is never time to catch up, so the backlog
and the response time keep growing the longer it runs. Skipping gives up some
jobs to keep the rest on time. Catching up keeps every job, but later and
later. For a logger, keeping every entry may be worth the delay. For a motor
controller, a late output is useless, so skipping is better.


===== FILE: lab3-periodic-rm/rm_taskset/rm_taskset.ino =====

```cpp
// rm_taskset - three periodic tasks under fixed priorities, theory vs board
//
// Each task is released every T ms and computes C ms (rt_busy_us). Its deadline
// is the next release. For each task set the sketch prints:
//   - the utilisation U and the Liu & Layland bound for n tasks
//   - the worst-case response time predicted by response-time analysis (RTA)
//   - what actually happened: response times, deadline misses, skipped jobs
//
// Keys pick the task set (edit the tables below to make your own):
//
//   a  U = 0.60, below the bound                  rate monotonic priorities
//   b  U = 0.85, above the bound                  rate monotonic priorities
//   c  U = 1.03, overload, late jobs are skipped  rate monotonic priorities
//   d  set b with the priorities reversed (the longest period gets the highest)
//   e  set c, but late jobs run late instead of being skipped (catch-up)
//
// All tasks are first released together at t0: the critical instant.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
struct Spec {
    const char* name;
    uint32_t T_ms;      // period, and relative deadline
    uint32_t C_ms;      // computation per job
    UBaseType_t prio;   // higher number = higher priority
};

const struct Spec kSetA[3] = {{"sensor", 10, 2, 6}, {"control", 20, 4, 5}, {"logger", 40, 8, 4}};
const struct Spec kSetB[3] = {{"sensor", 10, 3, 6}, {"control", 15, 4, 5}, {"logger", 35, 10, 4}};
const struct Spec kSetC[3] = {{"sensor", 10, 3, 6}, {"control", 15, 5, 5}, {"logger", 35, 14, 4}};
const struct Spec kSetD[3] = {{"sensor", 10, 3, 4}, {"control", 15, 4, 5}, {"logger", 35, 10, 6}};
const uint32_t kRunMs = 2100;         // 10 x the hyperperiod of sets b-e
const UBaseType_t kControlPrio = 10;  // loop() while it runs the experiment

struct Task {
    struct Spec s;
    // results
    uint32_t jobs, misses, skipped;
    int32_t r_max_us;
    int64_t r_sum_us;
    int32_t rta_us;  // -1 = RTA says the deadline can be missed
};

static struct Task g_task[3];
static bool g_catch_up;
static volatile bool g_stop;
static TickType_t g_t0_tick;
static int64_t g_t0_us;
static SemaphoreHandle_t g_done;

// Job k of a task is released at t0 + k * T. The same moment is needed twice:
// in microseconds, to measure the response time, and in ticks, to sleep.
static int64_t release_us(const struct Task* t, int64_t k) { return g_t0_us + k * (int64_t)t->s.T_ms * 1000; }

// Sleep until job k is released. If that moment has already passed, this
// returns at once, which is what makes the overrun policies below work.
static void sleep_until_release(const struct Task* t, int64_t k) {
    TickType_t T_ticks = pdMS_TO_TICKS(t->s.T_ms);
    rt_sleep_until_tick(g_t0_tick + (TickType_t)k * T_ticks);
}

// This is the whole periodic task: wait for the release, compute, note the
// response time, pick the next job.
static void periodic_task(void* arg) {
    struct Task* t = (struct Task*)arg;
    int64_t T_us = (int64_t)t->s.T_ms * 1000;
    int64_t k = 0;  // number of the current job

    sleep_until_release(t, k);
    while (!g_stop) {
        rt_busy_us(t->s.C_ms * 1000);                          // the job
        int32_t r = (int32_t)(rt_now_us() - release_us(t, k));  // its response time

        t->jobs++;
        t->r_sum_us += r;
        if (r > t->r_max_us) t->r_max_us = r;
        if (r > T_us) t->misses++;  // deadline = next release
        k++;

        // Overrun policy "skip": jobs whose release has already passed are
        // dropped; go straight to the next release in the future.
        // With "catch-up" they stay, and the sleep below returns at once.
        if (!g_catch_up) {
            while (release_us(t, k) <= rt_now_us()) {
                k++;
                t->skipped++;
            }
        }
        sleep_until_release(t, k);
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// Response-time analysis: R = C_i + sum over higher-priority j of
// ceil(R / T_j) * C_j, iterated until it stops changing.
static int32_t rta(int i, int n) {
    int32_t C = g_task[i].s.C_ms * 1000, D = g_task[i].s.T_ms * 1000;
    int32_t R = C, prev = 0;
    while (R != prev && R <= D) {
        prev = R;
        R = C;
        for (int j = 0; j < n; j++) {
            if (g_task[j].s.prio <= g_task[i].s.prio) continue;
            int32_t Tj = g_task[j].s.T_ms * 1000;
            R += ((prev + Tj - 1) / Tj) * (int32_t)(g_task[j].s.C_ms * 1000);
        }
    }
    return R <= D ? R : -1;
}

// Everything below only prints. One table row per task, then the verdict:
// SCHEDULABLE, or the number of misses of each task.
static void print_results(int n) {
    Serial.println("task     T ms  C ms  prio     U | RTA R ms | meas. R max  R avg |  jobs  miss  skip");
    uint32_t misses = 0;
    char v[96] = "MISSES";
    for (int i = 0; i < n; i++) {
        struct Task* t = &g_task[i];
        char rta_txt[12];
        if (t->rta_us < 0) snprintf(rta_txt, sizeof(rta_txt), "  > D");
        else snprintf(rta_txt, sizeof(rta_txt), "%6.2f", t->rta_us / 1000.0);
        float r_avg_ms = t->jobs ? (float)(t->r_sum_us / t->jobs) / 1000.0f : 0.0f;

        Serial.printf("%-8s %4lu  %4lu  %4u  %.3f |", t->s.name, (unsigned long)t->s.T_ms,
                      (unsigned long)t->s.C_ms, (unsigned)t->s.prio, t->s.C_ms / (float)t->s.T_ms);
        Serial.printf("   %s |     %6.2f %6.2f |", rta_txt, t->r_max_us / 1000.0, r_avg_ms);
        Serial.printf(" %5lu %5lu %5lu\n", (unsigned long)t->jobs, (unsigned long)t->misses,
                      (unsigned long)t->skipped);

        misses += t->misses;
        // add " name=misses" to the end of the verdict text
        size_t len = strlen(v);
        snprintf(v + len, sizeof(v) - len, " %s=%lu", t->s.name, (unsigned long)t->misses);
    }
    rt_verdict(misses == 0 ? "SCHEDULABLE" : v);
}

static void run(char key) {
    const struct Spec* set = kSetA;
    if (key == 'b') set = kSetB;
    if (key == 'c' || key == 'e') set = kSetC;
    if (key == 'd') set = kSetD;
    g_catch_up = (key == 'e');
    const int n = 3;

    float U = 0;
    for (int i = 0; i < n; i++) {
        memset(&g_task[i], 0, sizeof(struct Task));
        g_task[i].s = set[i];
        U += set[i].C_ms / (float)set[i].T_ms;
    }
    float bound = n * (powf(2.0f, 1.0f / n) - 1.0f);
    for (int i = 0; i < n; i++) g_task[i].rta_us = rta(i, n);

    Serial.printf("\n--- task set %c, overrun policy: %s ---\n", key, g_catch_up ? "catch-up" : "skip");
    Serial.printf("U = %.3f, Liu & Layland bound for %d tasks = %.3f -> %s\n", U, n, bound,
                  U <= bound ? "schedulable (sufficient test)" : "the bound test says nothing");

    // Release all tasks together at t0, 20 ms from now.
    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, kControlPrio);
    g_stop = false;
    g_t0_tick = xTaskGetTickCount() + pdMS_TO_TICKS(20);
    for (int i = 0; i < n; i++) rt_start_task(periodic_task, g_task[i].s.name, g_task[i].s.prio, &g_task[i]);
    rt_sleep_until_tick(g_t0_tick);  // wake at t0, before the tasks (higher priority)
    g_t0_us = rt_now_us();
    vTaskDelay(pdMS_TO_TICKS(kRunMs));
    g_stop = true;
    for (int i = 0; i < n; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskPrioritySet(NULL, my_prio);

    print_results(n);
}

static void menu(void) {
    Serial.println("\nkeys: a U=0.60 | b U=0.85 | c overload, skip | d b with wrong priorities | e overload, catch-up");
}

void setup() {
    rt_begin("lab 3: rm_taskset");
    g_done = xSemaphoreCreateCounting(3, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'e') {
        run((char)k);
        menu();
    }
    delay(20);
}
```


===== FILE: lab3-periodic-rm/wcet_measure/wcet_measure.ino =====

```cpp
// wcet_measure - how long does a job take? Measure it, many times.
//
// The job is an insertion sort of 64 numbers followed by a moving average: a
// stand-in for "the computation of a periodic task". It is timed with the CPU
// cycle counter (1 cycle = 1/160 us). Keys pick the variant:
//
//   a  1000 runs on the same input, interrupts on
//   b  1000 runs on the same input, interrupts off during each run
//   c  sorted, random and reverse-sorted input, 500 runs each, interrupts off
//      (here "random" is a new random input in every run)
//   d  like b, but before every run the flash cache is flushed by reading a
//      large table, so the job's code comes from flash again
//
// a, b and d sort the same input every time (random numbers, made once at
// start-up), so the input cannot be what changes the time. The very first run
// after reset is reported separately: code and constants that were never used
// are not in the cache yet.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const int kN = 64;          // elements to sort
const int kRuns = 1000;
const int kWindow = 8;      // moving average length

static int16_t g_in[kN];
static int16_t g_fixed[kN];  // the input of a, b and d, made once in setup()
static int16_t g_buf[kN];
static int32_t g_out[kN];
static uint32_t g_cyc[kRuns];
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;  // needed by portENTER_CRITICAL (interrupts off)

// 64 KiB of constants. They live in flash; the C3's cache is 16 KiB, so reading
// them all pushes everything else out of the cache.
static const uint8_t kBigTable[65536] = {1, 2, 3};

// ------------------------------------------------------------- the job ----

// noinline keeps job() a real function call, so the compiler cannot merge it
// into the timing code around it and optimise the two together.
static void __attribute__((noinline)) job(void) {
    for (int i = 0; i < kN; i++) g_buf[i] = g_in[i];
    // Insertion sort: the number of moves depends on how unsorted the input is.
    for (int i = 1; i < kN; i++) {
        int16_t x = g_buf[i];
        int j = i - 1;
        while (j >= 0 && g_buf[j] > x) {
            g_buf[j + 1] = g_buf[j];
            j--;
        }
        g_buf[j + 1] = x;
    }
    int32_t sum = 0;
    for (int i = 0; i < kN; i++) {
        sum += g_buf[i];
        if (i >= kWindow) sum -= g_buf[i - kWindow];
        g_out[i] = sum / kWindow;
    }
}

// ------------------------------------------------------------ helpers -----

// Input patterns. (Plain ints: the Arduino IDE generates function prototypes
// above the code, where an enum declared here would not be known yet.)
// kFixed is the same random input every time, kRandom a new one every time.
const int kRandom = 0, kSorted = 1, kReverse = 2, kFixed = 3;

static void make_input(int kind) {
    for (int i = 0; i < kN; i++) {
        if (kind == kSorted) g_in[i] = (int16_t)(i * 10);
        else if (kind == kReverse) g_in[i] = (int16_t)((kN - i) * 10);
        else if (kind == kFixed) g_in[i] = g_fixed[i];
        else g_in[i] = (int16_t)random(-1000, 1000);
    }
}

static void flush_cache(void) {
    // Read through a volatile pointer: the compiler knows the table's values
    // and would otherwise skip the reads.
    const volatile uint8_t* p = kBigTable;
    volatile uint32_t sink = 0;
    for (uint32_t i = 0; i < sizeof(kBigTable); i += 32) sink = sink + p[i];  // one read per cache line
}

static uint32_t time_one(bool irq_off) {
    uint32_t c0, c;
    if (irq_off) {
        portENTER_CRITICAL(&g_mux);
        c0 = rt_cycles();
        job();
        c = rt_cycles() - c0;
        portEXIT_CRITICAL(&g_mux);
    } else {
        c0 = rt_cycles();
        job();
        c = rt_cycles() - c0;
    }
    return c;
}

// Run the job n times and print min / avg / max in microseconds.
static void measure(const char* label, int kind, int n, bool irq_off, bool flush, char* verdict_out) {
    for (int r = 0; r < n; r++) {
        make_input(kind);
        if (flush) flush_cache();
        g_cyc[r] = time_one(irq_off);
    }

    struct rt_stats s;
    rt_stats_init(&s, 1);
    for (int r = 0; r < n; r++) rt_stats_add(&s, (int32_t)g_cyc[r]);
    Serial.printf("%-9s %5d runs: min %7.2f us, avg %7.2f us, max %7.2f us, max/avg %.2f\n", label, n,
                  s.min / (float)CPU_MHZ, rt_stats_avg(&s) / (float)CPU_MHZ, s.max / (float)CPU_MHZ,
                  s.max / (float)rt_stats_avg(&s));
    if (verdict_out) sprintf(verdict_out, "WCET_US=%.1f AVG_US=%.1f", s.max / (float)CPU_MHZ, rt_stats_avg(&s) / (float)CPU_MHZ);
    if (n == kRuns) {
        // The histogram shows how much slower than the fastest run each run
        // was, so the few slow runs stand out.
        struct rt_stats extra;
        rt_stats_init(&extra, 320);  // 320 cycles = 2 us per bucket
        for (int r = 0; r < n; r++) rt_stats_add(&extra, (int32_t)(g_cyc[r] - s.min));
        Serial.println("histogram: cycles above the fastest run (160 cycles = 1 us)");
        rt_stats_print_hist(&extra, "cyc");
    }
}

static void run(char key) {
    char v[64] = "";
    Serial.printf("\n--- variant %c ---\n", key);
    if (key == 'a') measure("same", kFixed, kRuns, false, false, v);
    if (key == 'b') measure("same", kFixed, kRuns, true, false, v);
    if (key == 'c') {
        measure("sorted", kSorted, 500, true, false, NULL);
        measure("random", kRandom, 500, true, false, NULL);
        measure("reverse", kReverse, 500, true, false, v);
    }
    if (key == 'd') measure("cold", kFixed, kRuns, true, true, v);
    rt_verdict(v);
}

static void menu(void) {
    Serial.println("\nkeys: a irq on | b irq off | c input patterns | d cold cache");
}

void setup() {
    rt_begin("lab 3: wcet_measure");
    // Measure at priority 5, above everything else the core starts except
    // the esp_timer and Wi-Fi tasks (loop() itself normally runs at 1).
    vTaskPrioritySet(NULL, 5);

    // The input of a, b and d. A fixed seed makes it the same on every board.
    randomSeed(42);
    for (int i = 0; i < kN; i++) g_fixed[i] = (int16_t)random(-1000, 1000);

    // The first run ever: nothing of job() is in the cache yet.
    make_input(kFixed);
    uint32_t first = time_one(true);
    uint32_t second = time_one(true);
    Serial.printf("first run after reset: %.2f us, second run: %.2f us\n", first / (float)CPU_MHZ,
                  second / (float)CPU_MHZ);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'd') {
        run((char)k);
        menu();
    }
    delay(20);
}
```


===== FILE: lab4-interrupts/README.md =====

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


===== FILE: lab4-interrupts/button/button.ino =====

```cpp
// button - a real input: the BOOT button, its interrupt and its bounce
//
// A mechanical contact does not switch cleanly. For a few hundred
// microseconds it closes and opens many times ("bounce"), and every one of
// those edges is an interrupt.
//
// The ISR does as little as possible: it counts the edge, notes the time and
// wakes a task. The task does the rest (deferred processing): it waits until
// the contact has settled, reads the pin once, and only then decides whether
// the button was pressed. Each debounced press toggles the LED.
//
//   press the BOOT button a few times, then look at the counters
//   r  reset the counters
//   v  print the verdict line
//
// Do not hold BOOT while pressing RESET or plugging in: that starts the
// bootloader instead of this sketch.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kSettleMs = 20;  // how long the contact must be quiet

// Written by the ISR. Times are rt_now_us() cut to 32 bits (wraps after 71
// minutes; differences stay correct), because the ISR writes them in one go
// and loop() reads them in one go. The cycle counter would not do here: it
// stops while the CPU sleeps, and between two edges it usually does.
static volatile uint32_t g_edges = 0;
static volatile uint32_t g_last_edge_us = 0;
static volatile uint32_t g_min_gap_us = 0xFFFFFFFFu;  // shortest time between two edges
// Written by the debounce task
static volatile uint32_t g_presses = 0;
static volatile uint32_t g_releases = 0;

static TaskHandle_t g_debounce = NULL;

static void ARDUINO_ISR_ATTR on_edge(void) {
    uint32_t t = (uint32_t)rt_now_us();
    if (g_edges > 0 && t - g_last_edge_us < g_min_gap_us) g_min_gap_us = t - g_last_edge_us;
    g_last_edge_us = t;
    g_edges = g_edges + 1;

    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(g_debounce, &woken);
    portYIELD_FROM_ISR(woken);
}

static void debounce_task(void* arg) {
    (void)arg;  // not used
    int stable = digitalRead(BOOT_PIN);  // HIGH = released
    bool led_on = false;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // some edge happened
        // Wait until no new edge has arrived for kSettleMs.
        while (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kSettleMs)) > 0) {
        }
        int level = digitalRead(BOOT_PIN);
        if (level == stable) continue;  // bounced and came back: no change
        stable = level;
        if (level == LOW) {
            g_presses = g_presses + 1;
            led_on = !led_on;
            digitalWrite(LED_PIN, led_on ? LOW : HIGH);  // the LED is lit when LOW
        } else {
            g_releases = g_releases + 1;
        }
    }
}

static void print_counters(void) {
    Serial.printf("edges %4lu | debounced presses %3lu, releases %3lu | edges per press+release %.1f | "
                  "shortest gap between edges %s",
                  (unsigned long)g_edges, (unsigned long)g_presses, (unsigned long)g_releases,
                  (g_presses + g_releases) ? g_edges / (float)(g_presses + g_releases) : 0.0f,
                  g_min_gap_us == 0xFFFFFFFFu ? "-" : "");
    if (g_min_gap_us != 0xFFFFFFFFu) Serial.printf("%lu us", (unsigned long)g_min_gap_us);
    Serial.println();
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // off
    pinMode(BOOT_PIN, INPUT_PULLUP);
    rt_begin("lab 4: button");

    g_debounce = rt_start_task(debounce_task, "debounce", 5, NULL);
    attachInterrupt(BOOT_PIN, on_edge, CHANGE);
    Serial.println("press the BOOT button; keys: r reset counters | v verdict");
}

void loop() {
    static uint32_t shown_edges = 0, shown_presses = 0, shown_releases = 0;
    // Print only when something changed, and only after the button has been
    // quiet for a moment, so printing never competes with an ongoing bounce.
    if ((g_edges != shown_edges || g_presses != shown_presses || g_releases != shown_releases) &&
        (uint32_t)rt_now_us() - g_last_edge_us > 100u * 1000) {
        shown_edges = g_edges;
        shown_presses = g_presses;
        shown_releases = g_releases;
        print_counters();
    }

    int k = rt_read_key();
    if (k == 'r') {
        noInterrupts();
        g_edges = 0;
        g_presses = 0;
        g_releases = 0;
        g_min_gap_us = 0xFFFFFFFFu;
        interrupts();
        shown_edges = shown_presses = shown_releases = 0;
        Serial.println("counters reset");
    } else if (k == 'v') {
        char v[64];
        snprintf(v, sizeof(v), "EDGES=%lu PRESSES=%lu", (unsigned long)g_edges, (unsigned long)g_presses);
        rt_verdict(v);
    }
    delay(10);
}
```


===== FILE: lab4-interrupts/isr_latency/isr_latency.ino =====

```cpp
// isr_latency - from a hardware event to the ISR, and from the ISR to a task
//
// A hardware timer counts at 10 MHz and raises an alarm every 0.997 ms. At the
// alarm it restarts from 0, so the value the ISR reads on entry is exactly how
// long the ISR took to start: the interrupt latency (0.1 us resolution).
// The ISR then wakes a handler task with a task notification. The handler
// measures how long after the ISR it got to run: the ISR-to-task latency.
//
//   a  handler priority 10, ISR asks for a context switch (portYIELD_FROM_ISR)
//   b  like a, but the ISR does not ask for the switch, while a task at
//      priority 1 computes all the time
//   c  handler priority 2, while a task at priority 5 computes 3 ms, sleeps 2 ms
//   d  like a, while a task at priority 3 keeps disabling interrupts for
//      200 us at a time (a critical section)
//   e  like a, while a second timer's ISR does 300 us of work every 3.7 ms
//   f  like e, but that ISR only wakes a task (priority 5) that does the work
//
// Nothing is printed while measuring. The ISR and the handler store numbers in
// arrays; loop() prints them afterwards.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const int kSamples = 2000;          // about 1 ms apart: 2 s per run
const uint32_t kTimerHz = 10000000; // 10 MHz, 0.1 us per count
// 0.997 ms, not 1 ms. The timer and the 1 ms tick run from the same crystal,
// so with exactly 1 ms every alarm would come at the same point between two
// ticks. 3 us less makes the alarm slide past the tick and meet it at every
// point (that matters in variant b).
const uint32_t kAlarmCounts = 9970;
const uint32_t kCriticalUs = 200;   // variant d
const uint32_t kNoisyWorkUs = 300;  // variants e and f
const uint32_t kNoisyPeriodUs = 3700;

static hw_timer_t* g_timer = NULL;  // the timer we measure
static hw_timer_t* g_noisy = NULL;  // the other one, variants e and f
static TaskHandle_t g_handler = NULL;
static TaskHandle_t g_worker = NULL;

static volatile bool g_yield;
static volatile bool g_noisy_defers;
static volatile bool g_stop;
static SemaphoreHandle_t g_done;

// Written by the ISR
static uint16_t g_isr_counts[kSamples];  // timer counts at ISR entry
static uint32_t g_isr_cycles[kSamples];  // cycle counter at ISR entry
static volatile int g_isr_n;
// Written by the handler task
static uint32_t g_task_cycles[kSamples];  // ISR entry -> handler running
static volatile int g_task_n;
static int g_consumed;                    // events the handler has dealt with
static volatile uint32_t g_merged;        // events that shared a wake-up

// ------------------------------------------------------------ ISRs --------

static void ARDUINO_ISR_ATTR on_alarm(void) {
    uint32_t counts = (uint32_t)timerRead(g_timer);  // first thing: latency
    uint32_t c = rt_cycles();
    int n = g_isr_n;
    if (n >= kSamples) return;
    g_isr_counts[n] = (uint16_t)counts;
    g_isr_cycles[n] = c;
    g_isr_n = n + 1;

    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(g_handler, &woken);
    // woken is true if the handler has a higher priority than the task this
    // interrupt stopped. Only then does a switch at ISR exit make sense.
    if (g_yield) portYIELD_FROM_ISR(woken);
}

static void ARDUINO_ISR_ATTR on_noisy(void) {
    if (g_noisy_defers) {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(g_worker, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        rt_busy_us(kNoisyWorkUs);  // "do the work in the ISR"
    }
}

// ----------------------------------------------------------- tasks --------

// Bookkeeping for one wake-up of the handler. `pending` is how many interrupts
// arrived since the last wake-up. Normally 1. If the handler was late, several
// interrupts share one wake-up; the oldest of them has waited longest, so
// that is the one whose latency is stored.
static void record_wakeup(uint32_t pending, uint32_t woke_cycles) {
    int oldest = g_consumed;
    g_consumed += pending;
    if (pending > 1) g_merged += pending - 1;
    int n = g_task_n;
    if (n < kSamples && oldest < kSamples) {
        g_task_cycles[n] = woke_cycles - g_isr_cycles[oldest];
        g_task_n = n + 1;
    }
}

// The handler: sleep until the ISR notifies us, note the time, "handle".
static void handler_task(void* arg) {
    (void)arg;  // not used
    for (;;) {
        uint32_t pending = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t c = rt_cycles();
        record_wakeup(pending, c);
    }
}

static void worker_task(void* arg) {  // variant f: the noisy ISR's deferred work
    (void)arg;  // not used
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        rt_busy_us(kNoisyWorkUs);
    }
}

static void busy_task(void* arg) {  // variant c: computes 3 ms, sleeps 2 ms
    (void)arg;  // not used
    while (!g_stop) {
        rt_busy_us(3000);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void background_task(void* arg) {  // variant b: always computing, priority 1
    (void)arg;  // not used
    while (!g_stop) rt_busy_us(100);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void critical_task(void* arg) {  // variant d: interrupts off 200 us at a time
    (void)arg;  // not used
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    while (!g_stop) {
        portENTER_CRITICAL(&mux);
        rt_busy_us(kCriticalUs);
        portEXIT_CRITICAL(&mux);
        rt_busy_us(50);
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------- report --------

static void print_us(const char* what, const struct rt_stats* s, int32_t per_us) {
    Serial.printf("%-22s min %8.1f us, avg %8.1f us, max %8.1f us\n", what, s->min / (float)per_us,
                  rt_stats_avg(s) / (float)per_us, s->max / (float)per_us);
}

static void run(char key) {
    g_yield = (key != 'b');
    g_noisy_defers = (key == 'f');
    UBaseType_t handler_prio = (key == 'c') ? 2 : 10;

    Serial.printf("\n--- variant %c: handler priority %u, %s ---\n", key, (unsigned)handler_prio,
                  g_yield ? "ISR yields" : "ISR does NOT yield");

    // loop() must be able to stop the run even when a background task never
    // sleeps, so it runs above everything while it waits.
    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 15);

    g_isr_n = 0;
    g_task_n = 0;
    g_consumed = 0;
    g_merged = 0;
    g_stop = false;
    int helpers = 0;
    g_handler = rt_start_task(handler_task, "handler", handler_prio, NULL);
    if (key == 'b') {
        rt_start_task(background_task, "background", 1, NULL);
        helpers++;
    }
    if (key == 'c') {
        rt_start_task(busy_task, "busy", 5, NULL);
        helpers++;
    }
    if (key == 'd') {
        rt_start_task(critical_task, "critical", 3, NULL);
        helpers++;
    }
    if (key == 'e' || key == 'f') {
        if (key == 'f') g_worker = rt_start_task(worker_task, "worker", 5, NULL);
        g_noisy = timerBegin(1000000);
        if (g_noisy == NULL) rt_fatal("timerBegin failed");
        timerAttachInterrupt(g_noisy, on_noisy);
        timerAlarm(g_noisy, kNoisyPeriodUs, true, 0);
    }

    g_timer = timerBegin(kTimerHz);
    if (g_timer == NULL) rt_fatal("timerBegin failed");
    timerAttachInterrupt(g_timer, on_alarm);
    timerAlarm(g_timer, kAlarmCounts, true, 0);

    vTaskDelay(pdMS_TO_TICKS(kSamples + 50));  // the run

    timerEnd(g_timer);
    if (g_noisy) {
        timerEnd(g_noisy);
        g_noisy = NULL;
    }
    g_stop = true;
    for (int i = 0; i < helpers; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskDelete(g_handler);
    if (g_worker) {
        vTaskDelete(g_worker);
        g_worker = NULL;
    }
    vTaskPrioritySet(NULL, my_prio);

    struct rt_stats isr, task;
    rt_stats_init(&isr, 10);
    rt_stats_init(&task, 160);
    for (int i = 0; i < g_isr_n; i++) rt_stats_add(&isr, g_isr_counts[i]);
    for (int i = 0; i < g_task_n; i++) rt_stats_add(&task, (int32_t)g_task_cycles[i]);
    Serial.printf("%d interrupts, handler woke %d times (%lu events shared a wake-up)\n", g_isr_n, g_task_n,
                  (unsigned long)g_merged);
    print_us("interrupt latency", &isr, kTimerHz / 1000000);
    print_us("ISR -> handler task", &task, CPU_MHZ);

    char v[64];
    snprintf(v, sizeof(v), "ISR_MAX_US=%.1f TASK_MAX_US=%.1f", isr.max / (float)(kTimerHz / 1000000),
             task.max / (float)CPU_MHZ);
    rt_verdict(v);
}

static void menu(void) {
    Serial.println("\nkeys: a baseline | b no yield | c low-priority handler | d critical sections | "
                   "e long ISR | f deferred ISR work");
}

void setup() {
    rt_begin("lab 4: isr_latency");
    g_done = xSemaphoreCreateCounting(2, 0);

    // Reading the timer is not free, and its cost is in every number below.
    hw_timer_t* t = timerBegin(kTimerHz);
    timerRead(t);  // once to get it into the cache
    uint32_t c0 = rt_cycles();
    volatile uint64_t x = timerRead(t);
    uint32_t c = rt_cycles() - c0;
    (void)x;
    timerEnd(t);
    Serial.printf("one timerRead() costs %.2f us\n", c / (float)CPU_MHZ);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'f') {
        run((char)k);
        menu();
    }
    delay(20);
}
```


===== FILE: lab5-shared-resources/README.md =====

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


===== FILE: lab5-shared-resources/deadlock/deadlock.ino =====

```cpp
// deadlock - two tasks, two mutexes, opposite order
//
//   task A: take m1, work, take m2, work, give both
//   task B: take m2, work, take m1, work, give both      <- opposite order
//
//   a  wait forever (portMAX_DELAY) and a 20 ms pause between the two takes:
//      deadlock. After 1 s the sketch shows who holds what and who waits.
//   b  the same, but every take gives up after 100 ms. A task that times out
//      releases what it holds, waits a random moment and starts again.
//   c  global lock order: B also takes m1 first. No deadlock possible.
//   d  no pause between the takes, 200 rounds each, 50 ms timeout: count how
//      often the deadlock happens when it depends on timing alone.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const UBaseType_t kPrio = 3;  // A and B have the same priority

struct Worker {
    char name;
    SemaphoreHandle_t first;   // the mutex this task takes first
    SemaphoreHandle_t second;  // ... and second
    TickType_t timeout;   // portMAX_DELAY = wait forever
    uint32_t pause_ms;    // between taking the first and the second mutex
    int rounds;
    // results
    int done_rounds;
    int timeouts;
    TaskHandle_t handle;
};

static SemaphoreHandle_t g_m1, g_m2;
static SemaphoreHandle_t g_done;
static struct Worker g_a, g_b;

static void worker_task(void* arg) {
    struct Worker* w = (struct Worker*)arg;
    while (w->done_rounds < w->rounds) {
        if (xSemaphoreTake(w->first, w->timeout) != pdTRUE) {
            w->timeouts++;
            continue;
        }
        if (w->pause_ms) vTaskDelay(pdMS_TO_TICKS(w->pause_ms));
        else rt_busy_us(random(0, 500));
        if (xSemaphoreTake(w->second, w->timeout) != pdTRUE) {
            // Detected: we hold one and cannot get the other. Back off.
            w->timeouts++;
            xSemaphoreGive(w->first);
            vTaskDelay(pdMS_TO_TICKS(random(1, 30)));
            continue;
        }
        rt_busy_us(1000);  // work with both resources
        xSemaphoreGive(w->second);
        xSemaphoreGive(w->first);
        w->done_rounds++;
    }
    xSemaphoreGive(g_done);
    vTaskSuspend(NULL);  // loop() deletes us
}

static void setup_worker(struct Worker* w, char name, SemaphoreHandle_t first, SemaphoreHandle_t second,
                         TickType_t timeout, uint32_t pause_ms, int rounds) {
    w->name = name;
    w->first = first;
    w->second = second;
    w->timeout = timeout;
    w->pause_ms = pause_ms;
    w->rounds = rounds;
    w->done_rounds = 0;
    w->timeouts = 0;
}

static const char* holder_name(SemaphoreHandle_t m) {
    TaskHandle_t h = xSemaphoreGetMutexHolder(m);
    return h ? pcTaskGetName(h) : "nobody";
}

static const char* state_name(TaskHandle_t t) {
    switch (eTaskGetState(t)) {
        case eRunning: return "running";
        case eReady: return "ready";
        case eBlocked: return "blocked";
        case eSuspended: return "suspended (finished)";
        default: return "deleted";
    }
}

static void run(char key) {
    g_m1 = xSemaphoreCreateMutex();
    g_m2 = xSemaphoreCreateMutex();
    TickType_t forever = portMAX_DELAY;
    if (key == 'a') {
        setup_worker(&g_a, 'A', g_m1, g_m2, forever, 20, 1);
        setup_worker(&g_b, 'B', g_m2, g_m1, forever, 20, 1);
    } else if (key == 'b') {
        setup_worker(&g_a, 'A', g_m1, g_m2, pdMS_TO_TICKS(100), 20, 1);
        setup_worker(&g_b, 'B', g_m2, g_m1, pdMS_TO_TICKS(100), 20, 1);
    } else if (key == 'c') {
        setup_worker(&g_a, 'A', g_m1, g_m2, forever, 20, 1);
        setup_worker(&g_b, 'B', g_m1, g_m2, forever, 20, 1);
    } else {
        setup_worker(&g_a, 'A', g_m1, g_m2, pdMS_TO_TICKS(50), 0, 200);
        setup_worker(&g_b, 'B', g_m2, g_m1, pdMS_TO_TICKS(50), 0, 200);
    }
    Serial.printf("\n--- variant %c ---\n", key);

    // Create both before either starts. Otherwise A, created first, would run
    // above loop() and could finish all its rounds before B even exists.
    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 10);
    int64_t t0 = rt_now_us();
    g_a.handle = rt_start_task(worker_task, "A", kPrio, &g_a);
    g_b.handle = rt_start_task(worker_task, "B", kPrio, &g_b);
    vTaskPrioritySet(NULL, my_prio);  // now they run

    // Wait for both, but not forever: that is the deadlock detector.
    int finished = 0;
    uint32_t wait_ms = (key == 'd') ? 30000 : 1000;
    while (finished < 2 && xSemaphoreTake(g_done, pdMS_TO_TICKS(wait_ms)) == pdTRUE) finished++;
    float took = (rt_now_us() - t0) / 1000.0f;

    if (finished < 2) {
        Serial.printf("after %lu s not both tasks have finished. Who holds what:\n", (unsigned long)(wait_ms / 1000));
        Serial.printf("  m1 is held by %s, m2 is held by %s\n", holder_name(g_m1), holder_name(g_m2));
        Serial.printf("  A is %s, B is %s\n", state_name(g_a.handle), state_name(g_b.handle));
        Serial.println("  A waits for m2 (held by B), B waits for m1 (held by A): a cycle.");
    } else {
        Serial.printf("both finished after %.1f ms\n", took);
    }
    Serial.printf("A: %d/%d rounds, %d timeouts | B: %d/%d rounds, %d timeouts\n", g_a.done_rounds, g_a.rounds,
                  g_a.timeouts, g_b.done_rounds, g_b.rounds, g_b.timeouts);

    char v[48];
    if (finished < 2) snprintf(v, sizeof(v), "DEADLOCK");
    else if (g_a.timeouts + g_b.timeouts > 0) snprintf(v, sizeof(v), "RECOVERED timeouts=%d", g_a.timeouts + g_b.timeouts);
    else snprintf(v, sizeof(v), "NO_DEADLOCK");
    rt_verdict(v);

    // Clean up. A deadlocked task can only be removed from outside.
    vTaskDelete(g_a.handle);
    vTaskDelete(g_b.handle);
    vSemaphoreDelete(g_m1);
    vSemaphoreDelete(g_m2);
}

static void menu(void) {
    Serial.println("\nkeys: a deadlock | b timeout and back off | c lock order | d timing-dependent, 200 rounds");
}

void setup() {
    rt_begin("lab 5: deadlock");
    g_done = xSemaphoreCreateCounting(2, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'd') {
        run((char)k);
        menu();
    }
    delay(20);
}
```


===== FILE: lab5-shared-resources/inversion/inversion.ino =====

```cpp
// inversion - priority inversion, and priority inheritance fixing it
//
// Three tasks share one CPU. L and H share a lock; M does not use it.
//
//    0 ms  L (priority 2) takes the lock and computes 40 ms inside it
//   10 ms  H (priority 5) wants the lock and has to wait for L
//   20 ms  M (priority 3) computes 150 ms, no lock
//
// If the lock is a binary semaphore, M preempts L, so H waits for M as well:
// H is blocked by a task with a lower priority that has nothing to do with the
// lock. With a FreeRTOS mutex, L inherits H's priority while H waits, so M
// cannot get in between, and H waits at most for the rest of L's critical
// section.
//
//   a  binary semaphore (no priority inheritance)
//   b  mutex (priority inheritance)
//   c  binary semaphore, and a second medium task N (priority 4) at 25 ms
//   d  mutex, with N as well
//
// "blocked" is the time from H asking for the lock until H getting it.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kCsMs = 40;       // L's critical section
const uint32_t kMediumMs = 150;  // each medium task's computation
const uint32_t kHighMs = 5;      // H's work inside the lock
const UBaseType_t kPrioL = 2, kPrioM = 3, kPrioN = 4, kPrioH = 5;

static SemaphoreHandle_t g_lock;
static SemaphoreHandle_t g_done;
static float g_h_request_ms, g_h_got_ms;
static UBaseType_t g_l_max_prio;  // highest priority L ran at inside the lock
static char g_letter_m = 'M', g_letter_n = 'N';  // timeline letters of the medium tasks

static void low_task(void* arg) {
    (void)arg;  // not used
    xSemaphoreTake(g_lock, portMAX_DELAY);
    for (uint32_t ms = 0; ms < kCsMs; ms++) {
        rt_tl_busy(1000, 'L');
        UBaseType_t p = uxTaskPriorityGet(NULL);
        if (p > g_l_max_prio) g_l_max_prio = p;
    }
    xSemaphoreGive(g_lock);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void medium_task(void* arg) {
    char letter = *(char*)arg;  // arg points to g_letter_m or g_letter_n
    rt_tl_busy(kMediumMs * 1000, letter);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void high_task(void* arg) {
    (void)arg;  // not used
    g_h_request_ms = rt_tl_ms();
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_h_got_ms = rt_tl_ms();
    rt_tl_busy(kHighMs * 1000, 'H');
    xSemaphoreGive(g_lock);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void wait_until_ms(int ms) {
    int32_t left = ms - (int32_t)rt_tl_ms();
    if (left > 0) vTaskDelay(pdMS_TO_TICKS(left));
}

static void run(char key) {
    bool mutex = (key == 'b' || key == 'd');
    bool two = (key == 'c' || key == 'd');
    if (mutex) {
        g_lock = xSemaphoreCreateMutex();
    } else {
        g_lock = xSemaphoreCreateBinary();
        xSemaphoreGive(g_lock);  // a binary semaphore starts empty
    }
    g_l_max_prio = 0;

    Serial.printf("\n--- variant %c: %s, %s ---\n", key, mutex ? "mutex (priority inheritance)" : "binary semaphore",
                  two ? "two medium tasks" : "one medium task");

    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 10);  // releases happen on time
    rt_tl_start(5000, 80);
    rt_start_task(low_task, "L", kPrioL, NULL);
    wait_until_ms(10);
    rt_start_task(high_task, "H", kPrioH, NULL);
    wait_until_ms(20);
    rt_start_task(medium_task, "M", kPrioM, &g_letter_m);
    if (two) {
        wait_until_ms(25);
        rt_start_task(medium_task, "N", kPrioN, &g_letter_n);
    }
    for (int i = 0; i < (two ? 4 : 3); i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskPrioritySet(NULL, my_prio);
    vSemaphoreDelete(g_lock);

    rt_tl_print(80);
    float blocked = g_h_got_ms - g_h_request_ms;
    Serial.printf("H asked for the lock at %.1f ms and got it at %.1f ms: blocked %.1f ms\n", g_h_request_ms,
                  g_h_got_ms, blocked);
    Serial.printf("L's critical section is %lu ms; L ran at priority up to %u while holding the lock\n",
                  (unsigned long)kCsMs, (unsigned)g_l_max_prio);
    rt_verdict(blocked <= kCsMs + 1 ? "BOUNDED" : "UNBOUNDED");
}

static void menu(void) {
    Serial.println("\nkeys: a binary semaphore | b mutex | c semaphore, two mediums | d mutex, two mediums");
}

void setup() {
    rt_begin("lab 5: inversion");
    g_done = xSemaphoreCreateCounting(4, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'd') {
        run((char)k);
        menu();
    }
    delay(20);
}
```


===== FILE: lab5-shared-resources/race/race.ino =====

```cpp
// race - two tasks, one counter, lost updates
//
// Two tasks each add 1 to the same counter one million times. The result
// should be 2 000 000. `counter = counter + 1` is three instructions: load,
// add, store. If the scheduler switches tasks between the load and the store,
// the other task's work in the meantime is overwritten: a lost update.
//
//   a  no protection, both tasks at priority 3 (the tick rotates them)
//   b  no protection, priorities 3 and 4
//   c  a FreeRTOS mutex around every increment, priority 3 and 3
//   d  a critical section (interrupts off) around every increment
//   e  an atomic add: load, add and store as one indivisible step
//
// Every variant prints the final count, how many updates were lost and how
// long the whole thing took.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kIncrements = 1000000;  // per task

static volatile uint32_t g_counter;   // variants a-d
static uint32_t g_atomic;             // variant e
static SemaphoreHandle_t g_mutex;
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;  // needed by portENTER_CRITICAL (interrupts off)
static SemaphoreHandle_t g_done;
static char g_mode;

static void adder_task(void* arg) {
    (void)arg;  // not used
    for (uint32_t i = 0; i < kIncrements; i++) {
        switch (g_mode) {
            case 'c':
                xSemaphoreTake(g_mutex, portMAX_DELAY);
                g_counter = g_counter + 1;
                xSemaphoreGive(g_mutex);
                break;
            case 'd':
                portENTER_CRITICAL(&g_mux);
                g_counter = g_counter + 1;
                portEXIT_CRITICAL(&g_mux);
                break;
            case 'e':
                // A compiler built-in: the whole read-add-write cannot be
                // interrupted halfway, and it needs no mutex.
                __sync_fetch_and_add(&g_atomic, 1);
                break;
            default:  // a, b: the race
                g_counter = g_counter + 1;
        }
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void run(char key) {
    g_mode = key;
    g_counter = 0;
    g_atomic = 0;
    UBaseType_t p1 = 3, p2 = (key == 'b') ? 4 : 3;

    Serial.printf("\n--- variant %c: priorities %u and %u ---\n", key, (unsigned)p1, (unsigned)p2);
    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 10);  // create both before either starts
    int64_t t0 = rt_now_us();
    rt_start_task(adder_task, "adder1", p1, NULL);
    rt_start_task(adder_task, "adder2", p2, NULL);
    vTaskPrioritySet(NULL, my_prio);  // now they run
    xSemaphoreTake(g_done, portMAX_DELAY);
    xSemaphoreTake(g_done, portMAX_DELAY);
    int64_t took = rt_now_us() - t0;

    uint32_t result = (key == 'e') ? g_atomic : g_counter;  // both adders have finished
    uint32_t expected = 2 * kIncrements;
    Serial.printf("expected %lu, got %lu, lost %lu updates (%.2f %%), took %.1f ms\n", (unsigned long)expected,
                  (unsigned long)result, (unsigned long)(expected - result), 100.0 * (expected - result) / expected,
                  took / 1000.0);
    char v[32];
    snprintf(v, sizeof(v), "LOST=%lu", (unsigned long)(expected - result));
    rt_verdict(v);
}

static void menu(void) {
    Serial.println("\nkeys: a race | b race, different priorities | c mutex | d critical section | e atomic");
}

void setup() {
    rt_begin("lab 5: race");
    g_mutex = xSemaphoreCreateMutex();
    g_done = xSemaphoreCreateCounting(2, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'e') {
        run((char)k);
        menu();
    }
    delay(20);
}
```
