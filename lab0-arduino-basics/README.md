# Lab 0 — Arduino basics

Before the real-time labs, the plain Arduino tools they are built from: the
`setup()` / `loop()` structure, the serial link to your computer, a digital
output (the LED), a digital input (the BOOT button), waiting with `delay()`
against waiting with `millis()`, and brightness by pulse-width modulation,
which you can also watch on a logic analyzer if you have one. Four short
sketches, one idea each. Nothing here needs FreeRTOS or `rt.h`:
it is the Arduino API as you would use it on any board.

This lab is about observing. Each experiment says what to press, what to look
at in the output, and what the result shows.

Time: about 1 hour, including setup.

Background for this lab:
[../docs/arduino_overview.md](../docs/arduino_overview.md) (what Arduino is
and how a sketch runs) and
[../docs/c_for_these_labs.md](../docs/c_for_these_labs.md) (the C you will
meet). There are no theory pages for it in
[rt-concepts](https://github.com/automatica-cluj/rt-concepts); those start
with Lab 1.

---

## 1. Setup

Follow [../docs/setup.md](../docs/setup.md), steps 1 and 2. For the first
upload use `hello_serial/hello_serial.ino` from this lab instead of the Lab 1
sketch named there: upload it, open the Serial Monitor at 115200 baud and
press RST. When `=== lab 0: hello_serial ===` appears, you are ready.

To send a key, type it in the Serial Monitor's input line and press Enter.
The sketches ignore the Enter itself.

The two parts on the board that this lab uses:

| | pin | level |
|---|---|---|
| Blue LED | GPIO 8 | lit when the pin is **LOW** |
| BOOT button | GPIO 9 | reads **LOW** while pressed |

Both are "active low": the LED is wired between 3.3 V and the pin, so it
lights when the pin pulls its end to ground, and the button connects the pin
to ground when pressed. Every sketch here writes this down once, in a small
helper, so the rest of the code can think in "on" and "off".

## 2. The structure of a sketch and the serial link: `hello_serial`

Upload `hello_serial/hello_serial.ino`.

`setup()` opens the serial port with `Serial.begin()`, waits up to 3 s for
the Serial Monitor (over USB the board is usually faster than your computer),
and prints the menu. Then `loop()` runs forever: it counts its own passes,
checks with `Serial.available()` whether a key has arrived, reads it with
`Serial.read()` and acts on it. A sketch never calls `setup()` or `loop()`
itself; the Arduino core does, as
[../docs/arduino_overview.md](../docs/arduino_overview.md) explains.

### Experiment A — the board (key `i`)

Press `i`. The sketch asks the core about the chip with the `ESP.` functions
and prints the answers.

**Look at** the chip model, the CPU clock, the free heap, the uptime and the
`loop() ran` count. Press `i` again a few seconds later.

**What it shows.** The board knows what it is: an ESP32-C3 at 160 MHz, with
a few hundred kilobytes of RAM free. That is little next to a PC, and it is
the budget every lab sketch lives in. Between two presses the uptime has
grown by about the time you waited, and the loop count by roughly 100 per
second, because `loop()` ends with `delay(10)`. The temperature is the chip's
own, measured on the die. It is a rough value and runs above the room
temperature.

### Experiment B — C types on this chip (key `t`)

Press `t`.

**Look at** the sizes, then the five calculations below them.

**What it shows.** On this 32-bit chip `int`, `long` and a pointer are all
4 bytes. (On a PC, `long` and pointers are usually 8, and on an Arduino Uno
`int` is 2.) That is why embedded code prefers `uint8_t`, `uint32_t` and
`int64_t`: their size is in the name and does not change between chips.

Unsigned types wrap around at their limit, with no error: `255 + 1` in a
`uint8_t` is 0, and `0 - 1` in a `uint32_t` is the largest value, 4294967295.
`millis()` returns a `uint32_t`, so it wraps after 49.7 days. The sketches
always compute `now - start`, and that difference stays right across the
wrap. Integer division drops the fraction (`7 / 2` is 3), and one `float`
operand is enough to keep it. A `float` holds only about 7 significant
digits, so it cannot tell 16777216 from 16777217.

### Experiment C — printing numbers (key `f`)

Press `f`.

**Look at** the same two numbers, 200 and 3.14159, printed in different
ways.

**What it shows.** `Serial.print(x, HEX)` and `Serial.print(x, BIN)` choose
the base, and `Serial.print(f, 4)` the number of decimals. `Serial.printf()`
takes the format strings of C's `printf`: `%d`, `%x`, `%f`, a width such as
`%5d` for aligned columns, and `%.2f` for the decimals. Every later lab
prints its tables with `Serial.printf()`.

### Experiment D — any other key

Type a letter, a digit, a space.

**Look at** the character code printed next to each one.

**What it shows.** What travels over the serial link is bytes. A key is its
ASCII code: `a` is 97, `A` is 65, `0` is 48. The sketch compares the byte it
read against `'i'`, `'t'` and `'f'`, which are those same numbers written as
characters. When you press Enter the Serial Monitor sends the line-end bytes
too (10 and possibly 13); the sketch skips them.

## 3. A digital output, and two ways to wait: `blink`

Upload `blink/blink.ino`. The LED starts blinking, half a second on and half
a second off.

`pinMode(8, OUTPUT)` makes the pin an output, and `digitalWrite(8, LOW)` or
`HIGH` sets its level. The blinking itself is done without `delay()`. Each
pass of `loop()` asks one question: has half a period gone by since the last
toggle (`millis() - g_last_toggle_ms >= g_half_ms`)? If so, it toggles the
LED. Otherwise it goes on to read the keys. `loop()` has no `delay()` at the
end at all.

### Experiment E — `+` and `-`

Press `+` a few times, then `-`.

**Look at** the LED and the printed on/off times.

**What it shows.** The blink rate changes the moment you press the key.
Because `loop()` never waits, it notices the key at once, between two checks
of the clock.

### Experiment F — blinking with `delay()` (key `a`)

Press `a`, and while the LED blinks five times, type a few more letters and
press Enter.

**Look at** the line `loop() was stuck for ... ms; ... characters waited`.

**What it shows.** `blink_with_delay()` is the classic first sketch:
`digitalWrite`, `delay`, `digitalWrite`, `delay`. It works, but during each
`delay()` the sketch does nothing else. `loop()` is stuck for five whole
periods, and the keys you typed only wait in the input buffer. The sketch
throws them away afterwards, since they came too late. One task with
`delay()` is fine; two things at once, such as blinking and listening, need
the `millis()` style.

### Experiment G — how often `loop()` runs (key `l`)

Press `l`.

**Look at** the `LOOPS_PER_S=` count.

**What it shows.** Without a `delay()`, `loop()` comes back a very large
number of times per second, and the LED still blinks on time. Each pass is
short: a clock check, maybe a toggle, a look at the serial buffer. This is
how one `loop()` can serve several jobs, each with its own "is it time yet?"
check. It is also the limit of the style. If one job takes long, all the
others wait. From Lab 1 on, FreeRTOS tasks solve that differently: each job
gets its own task, and the scheduler switches between them.

Press `b` to stop or restart the `millis()` blinking.

## 4. A digital input: `button_poll`

Upload `button_poll/button_poll.ino`.

`pinMode(9, INPUT_PULLUP)` makes the pin an input and switches on a resistor
inside the chip that pulls it up to 3.3 V. Released, the button leaves the
pin to that resistor, and it reads `HIGH`. Pressed, it connects the pin to
ground, and it reads `LOW`. Without the pull-up a released pin would be
connected to nothing and could read either level.

Every `kPollMs` (10 ms), `loop()` reads the pin with `digitalRead()` and
compares the level with the one it saw last time. Only a change counts: HIGH
to LOW is a press, LOW to HIGH a release. This is **polling**: the program
goes and looks, again and again.

Do not hold BOOT while pressing RST: that starts the bootloader instead of
the sketch.

### Experiment H — presses and their duration

Press BOOT a few times: short taps, then a long press.

**Look at** the LED, and the line `press N, held for ... ms` printed at each
release.

**What it shows.** Each press toggles the LED once, and the sketch measures
how long you held the button by subtracting two `millis()` readings. Reading
the level alone is not enough: if the sketch toggled the LED whenever it read
`LOW`, the LED would flicker on and off for as long as you hold the button.
Detecting the *change* of level turns a held button into a single event.

### Experiment I — follow mode (key `m`)

Press `m`, then press and hold BOOT. Press `m` again to go back.

**Look at** the LED.

**What it shows.** In follow mode the LED is lit exactly while the button is
held: the output copies the input. The same edges drive both modes; only
what the sketch does with them differs.

`v` prints the press count as a verdict line, `r` resets it.

Optional: set `kPollMs = 0` and upload again. Now the pin is read on every
pass of `loop()`, many times per millisecond. A mechanical contact does not
switch cleanly, it **bounces** for a short moment, and read this fast, some
of your presses may now count twice. At 10 ms the sketch looks too seldom to
see the bounce. Lab 4 measures it and handles it properly, with an interrupt
and a debounce task.

## 5. Brightness from an on/off pin: `pwm_fade`

Upload `pwm_fade/pwm_fade.ino`.

A digital pin has only two levels. To make the LED dimmer, the pin is
switched on and off over and over, and the LED is lit for only part of each
period. The fraction of the period it is lit is the **duty cycle**. This is
**pulse-width modulation** (PWM).

### Experiment J — PWM by hand, slow (key `s`)

Press `s`. For 3 s the sketch switches the LED with `digitalWrite()` and
`delayMicroseconds()`: 5 periods per second, lit 25 % of each.

**Look at** the LED, and the period and on/off times printed.

**What it shows.** At 5 Hz you see exactly what the code does: a short flash,
a longer dark gap, five times a second.

### Experiment K — PWM by hand, fast (key `h`)

Press `h`. The same 25 % duty cycle, now at 500 Hz.

**Look at** the LED next to experiment J.

**What it shows.** The code is the same, only the period is 100 times
shorter. The eye can no longer follow the switching and sees a steady, dimmer
light instead. The pin still does nothing but switch fully on and fully off.
But to do it, the CPU did nothing else for 3 s: like `delay()` in experiment
F, PWM by hand keeps the program busy.

### Experiment L — hardware PWM (keys `0` to `9`, `f`)

Press `9`, then `5`, `3`, `1`, `0`. Then press `f`.

**Look at** the LED, and the `analogWrite(8, ...)` value printed for each
key.

**What it shows.** `analogWrite(pin, value)` hands the switching to
**LEDC**, a PWM peripheral inside the chip. It keeps switching the pin at
1 kHz on its own, and the CPU goes straight back to `loop()`. Each period has
256 steps, and the pin is HIGH for `value` of them (255 and above means HIGH
all the time). Because this LED lights when the pin is LOW, the sketch lights
it for `lit` steps by writing `256 - lit`: 0 % writes 256 (always HIGH, LED
off), 100 % writes 0 (always LOW, LED fully on). The digit keys are not evenly spaced (0, 1, 2, 5, 10, 20,
35, 50, 75 and 100 %), because equal steps of duty cycle do not look equal:
the eye notices a change from 1 % to 2 % far more than one from 75 % to
76 %. The fade (`f`) walks through every step from off to fully on and back
down.

Despite its name, `analogWrite()` produces no analog voltage. The pin still
switches between 0 and 3.3 V, and only the average comes out in between.

### Optional — watch it on a logic analyzer

This part needs a logic analyzer, for example a Saleae Logic 8 with the
Logic 2 application. The LED pin is on the header, marked `8`:

| Analyzer | Board pin | Signal |
|---|---|---|
| channel 0 | `8` (GPIO8) | the LED pin: LED lit while it is **LOW** |
| ground | `G` | |

On a Saleae, the channel wires are coloured like resistor bands: channel 0
is **black**, channel 1 brown. The grey wires are ground. Black is not
ground: clipping it to `G` leaves channel 0 empty. Take care that the clip on
`G` does not touch `5V` or `3V3` next to it. GPIO 8 is a strapping pin, but
the analyzer only listens and does not pull it either way, so the board
still boots normally with the clip on.

In Logic 2, in the Device Settings panel, enable digital channel 0 only and
set the digital sample rate to 1 to 10 MS/s, plenty for a 1 ms period. Use
the Looping capture mode, press Start, then press keys in the Serial
Monitor. Hovering over a pulse shows its width, and the Measurements panel
shows the period, frequency and duty cycle.

Remember that the signal is upside down compared with the LED: the LED is
lit while the line is **low**, so the duty cycle Logic 2 reports (the time
high) is 100 % minus the brightness.

**Keys `5`, `2`, `9`, `0`.**

**Look at** the period, and the width of the low part.

**What it shows.** The period is always 1 ms (1 kHz). Only the low part
changes: half the period at `5` (50 %), a narrow low pulse of about
5/256 of the period at `2` (2 %). At `9` the line stays low, at `0` it stays
high. The brightness you saw in experiment L is just how much of each
millisecond the line spends low.

**Key `f`.**

**Look at** the low pulses over the whole fade, then zoom in at a few
places.

**What it shows.** The low part widens step by step until it fills the
period, then narrows back, while the period never changes. A fade is nothing
but a duty cycle that changes over time.

**Keys `s` and `h`.**

**Look at** the period and the low time, then zoom in on a few edges and
compare them with key `5`.

**What it shows.** At `s` the line is low 50 ms out of every 200 ms (5 Hz,
25 %), and at `h` 0.5 ms out of every 2 ms (500 Hz, 25 %): the same shape at
two speeds, the one you saw blink and the one you saw dim. These edges are
made by `digitalWrite()` and `delayMicroseconds()` in the CPU, so anything
else the CPU does, such as an interrupt, can shift one of them a little. The
edges from LEDC come from a hardware counter, and nothing the CPU does moves
them.
