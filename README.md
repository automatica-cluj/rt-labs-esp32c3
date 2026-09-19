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
2. Read [docs/freertos_concepts.md](docs/freertos_concepts.md): tasks,
   priorities, the tick, interrupts, and how FreeRTOS lets you share data.
3. If your C is rusty: [docs/c_for_these_labs.md](docs/c_for_these_labs.md).

## Labs

| Lab | Topic | Sketches |
|---|---|---|
| [lab1-time-latency](lab1-time-latency/) | Clocks, latency and jitter, relative vs absolute periodic release, interference from a higher priority and from Wi-Fi | `clocks`, `periodic_jitter` |
| [lab2-scheduling](lab2-scheduling/) | Fixed-priority preemptive scheduling, time slicing, task states, starvation, the task watchdog | `scheduling`, `starvation_wdt` |
| [lab3-periodic-rm](lab3-periodic-rm/) | Measuring WCET, Rate Monotonic, utilisation bound, response-time analysis, overload and overrun policy | `wcet_measure`, `rm_taskset` |
| [lab4-interrupts](lab4-interrupts/) | Interrupt latency, ISR-to-task latency, `portYIELD_FROM_ISR`, critical sections, deferred processing, debouncing | `isr_latency`, `button` |
| [lab5-shared-resources](lab5-shared-resources/) | Race conditions, priority inversion and inheritance, deadlock detection and prevention | `race`, `inversion`, `deadlock` |

Each lab takes about two hours. Each sketch runs its experiments when you
press a key (`a`, `b`, ...) in the serial monitor, so most experiments need
no re-upload. Values you are invited to change sit at the top of the sketch
under `// ---- EXPERIMENT KNOBS ----`.

## How the sketches are built

- `common/rtlab.h` is a short header with the shared helpers: clocks
  (`now_us`, `cycles`), calibrated busy work (`burn_us`), task creation,
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
make sync                  # copy common/rtlab.h into every sketch folder
make compile               # compile all sketches, warnings on (no board needed)
make hil PORT=/dev/...     # flash each sketch, press its keys, check each VERDICT
```

`make compile` uses `arduino-cli` from `PATH`, or the copy inside the Arduino
IDE on macOS, with FQBN `esp32:esp32:esp32c3:CDCOnBoot=cdc`. It was tested
with arduino-esp32 3.3.7. `make hil` needs `pyserial` and a board. Its
expected verdicts live in `tools/hil_smoke.py`, and they are the behaviour the
READMEs promise. Keep the two in sync.

The READMEs describe what to look for instead of showing captured output.
When you run the labs on real boards, consider adding a short captured
sample to each experiment.
