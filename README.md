# Real-Time Systems — ESP32-C3 labs

Five lab sessions on real-time concepts, run on an **ESP32-C3 Super Mini**
under FreeRTOS, programmed with the Arduino IDE, plus an introductory Lab 0
with the Arduino basics. Each lab is a directory with a README and a few
sketches. All you need is the board and a USB-C
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
4. The theory behind each lab, in Romanian, lives in its own repository,
   shared with the Linux track: [rt-concepts](https://github.com/automatica-cluj/rt-concepts).
   Each lab README lists the pages to read before the session.

## Labs

| Lab | Topic | Sketches |
|---|---|---|
| [lab0-arduino-basics](lab0-arduino-basics/) | Arduino basics, no real-time yet: `setup()`/`loop()`, the serial link, C types, a digital output and input, `delay()` vs `millis()`, PWM, optionally seen on a logic analyzer | `hello_serial`, `blink`, `button_poll`, `pwm_fade` |
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
  `.ino`. Read it once in Lab 1. The Lab 0 sketches use plain Arduino and
  do not include it.
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
