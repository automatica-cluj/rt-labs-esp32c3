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
