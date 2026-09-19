# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

Teaching material for the UTCN Real-Time Systems lab: a standalone track of five
2-hour labs on an **ESP32-C3 Super Mini** (single-core RISC-V, 160 MHz),
programmed with the **Arduino core 3.x** (tested 3.3.7). The sketches call the
FreeRTOS API directly. The equipment is the board and USB only: all timing
comes from `esp_timer_get_time()`, the cycle counter and hardware timers,
reported over USB CDC serial. The sibling Linux/PREEMPT_RT track lives in
`../ssatr-iaisc-icaf-2025-labs-v2`, and its style is followed here.

Layout: `labN-topic/README.md` plus sketch folders `labN-topic/<name>/<name>.ino`,
each holding a **copy** of `common/rtlab.h`. `docs/` has the setup guide, the
FreeRTOS concepts page and the C primer (practical, English). `concepts/` has
the theory pages (Romanian, theory only), grouped in one folder per lab, plus
`index.md`, `glossary.md` and `further-reading.md`; each lab README links to
its pages. `tools/` has the sync, compile,
hardware-in-the-loop and plot scripts.

## Build and test

```bash
make sync                    # copy common/rtlab.h next to every .ino
make compile                 # sync, then compile every sketch with --warnings all (no board)
make hil PORT=/dev/cu.usbmodemXXXX                       # flash every sketch, press its keys, check verdicts
make hil PORT=/dev/cu.usbmodemXXXX ONLY="race inversion" # only these sketches (folder basenames)
```

One sketch without the Makefile (same settings as `compile_all.sh`):

```bash
ARDUINO_DIRECTORIES_USER=$PWD/.arduino-user arduino-cli compile \
  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc --warnings all lab5-shared-resources/race
```

- The scripts find `arduino-cli` on PATH or inside the macOS Arduino IDE
  bundle. `ARDUINO_CLI` and `FQBN` override the defaults.
- They set `ARDUINO_DIRECTORIES_USER=.arduino-user`, because the CLI hangs on
  the macOS privacy check when it touches `~/Documents/Arduino`.
- Warnings from the core's own headers are ignored; warnings in `.ino` or
  `rtlab.h` fail the build. A stale `rtlab.h` copy fails it too
  (`sync_common.sh --check`).
- `make hil` needs a board and `pyserial`. The full serial output of the run
  goes to `hil_smoke.log`. There are no other tests.
- `tools/plot_serial.py run.log [out.png]` (needs matplotlib) plots the
  `csv: k,...` / `csv: end` block that `periodic_jitter` prints on key `p`.

## How the pieces fit

- `common/rtlab.h` is the whole shared library: header-only, `static inline`
  functions, one global `Timeline g_tl`. It gives clocks (`now_us`, `cycles`),
  calibrated busy work (`burn_us`, `tl_burn`), `start_task`, `Stats`,
  the timeline strip, `read_key` and `verdict`. The Arduino IDE only finds
  headers next to the `.ino`, hence the copies. Both scripts pick sketches up
  with the glob `lab*/*/*.ino`, so a new sketch folder is synced and compiled
  with no further registration.
- `verdict()` prints `VERDICT: <text>`, and that line is the contract between
  a sketch, its README and `tools/hil_smoke.py`. For each entry in `CHECKS`
  the harness sends the key, then judges the **first** line that starts with
  `VERDICT:` against a regex. A second or stray verdict line is therefore
  read as the answer to the next key. The harness reopens the port if the
  board resets (the `starvation_wdt` variants that reset on purpose), and
  the key order in `CHECKS` is the order they are run in.
- `rtlab_begin()` waits up to 3 s for the serial monitor, then calibrates
  `burn_us()` at priority 20 with interrupts off. The harness sleeps 3.5 s
  after opening the port for this reason.
- A new sketch needs: its folder and `.ino`, `make sync`, a section in the lab
  README, a row entry in the root `README.md` table, and a `CHECKS` entry.

## Conventions

- Edit `common/rtlab.h` only, then run `make sync`. Never edit a copy.
- Code stays close to C: no lambdas, templates or classes with methods.
  References, `const`, structs and one `std::atomic` (Lab 5) are fine, and
  each is explained in `docs/c_for_these_labs.md`. Keep that page in sync.
- **Arduino prototype trap:** the IDE generates function prototypes above
  user type declarations, so functions must not take enums declared in the
  sketch. Use `int` constants. Structs declared before use have worked.
- Each sketch: header comment listing the keys, a `// ---- EXPERIMENT KNOBS ----`
  block, `rtlab_begin()` in `setup()`, variants selected by single keys in
  `loop()`, and one `VERDICT: ...` line per experiment.
- Nothing is printed inside timing loops. Tasks record into static arrays and
  `loop()` prints afterwards.
- Lab task priorities stay within 1–20 (esp_timer 22, Wi-Fi 23). The control
  code in `loop()` raises itself (usually to 10) while it releases tasks.
- `burn_us()` is iteration-based CPU time, calibrated at start-up with
  interrupts off, so preemption stretches it the way real computation would.
- Keep `portENTER_CRITICAL` sections far below 300 ms (the interrupt watchdog).
  The task watchdog is 5 s and does not watch the idle task.
  `starvation_wdt` shortens it to 3 s and adds `loopTask` on purpose.
- READMEs: goal and time, numbered sections, lettered experiments,
  Questions. Plain prose, no emojis. They describe what to look for and
  contain no fabricated numbers. Captured output from a real board may be
  added.
- `concepts/` holds theory pages for students, in **Romanian**, and they are
  **theory only**: no code, no API or function names, no sketch names, keys,
  verdicts or lab instructions, platform-neutral (a concrete value only as a
  "de exemplu"). Template: Problema, Ideea, Teorie, Exemplu (hand-calculated),
  Greșeli frecvente, Întrebări de verificare, a "Pentru mai mult" line, and a
  final "Laborator asociat" link, the only lab reference. 90 to 150 lines,
  few formulas in `$...$` LaTeX, no proofs, English file names, terminology
  from `concepts/glossary.md`: add new terms there, and keep `concepts/index.md`
  and the "Theory behind this lab" links in the lab READMEs in step when a
  page is added or renamed. To be published later as a static site (MkDocs).
- When changing a sketch, update its README and the expected verdicts in
  `tools/hil_smoke.py`.
