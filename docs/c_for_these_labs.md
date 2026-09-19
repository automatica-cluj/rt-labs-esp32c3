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

`arg` is whatever pointer you passed to `start_task()`. The labs pass a
pointer to a struct (`Worker* w = (Worker*)arg;`) or to a global variable
(`char letter = *(char*)arg;`). A task that needs no argument is written
`static void hog_task(void*)`: C++ lets you leave out the name of a parameter
you do not use, and the compiler then does not warn about it.

`start_task(fn, "name", prio)` is `xTaskCreate` with a check. Its last two
parameters, `arg` and `stack`, have **default values** (`NULL` and 4096), so
you only write them when you need them.

## Things you will see

**`static` in front of a function or global** means "private to this file".

**`const int kPeriods = 300;`** is a constant. The `k` prefix marks
constants. The block marked `// ---- EXPERIMENT KNOBS ----` at the top of each
sketch holds the values you are invited to change.

**References: `Stats& s`.** Another name for an existing variable. A parameter
`Stats& s` behaves like `Stats* s` in C, but you write `s.min` instead of
`s->min`, and it can never be null.

**`Stats s; stats_reset(s, 50);`** Structs are plain C structs with helper
functions next to them, not classes with methods.

**`portMUX_TYPE mux` and `portENTER_CRITICAL(&mux)`.** A critical section:
interrupts are off until `portEXIT_CRITICAL(&mux)`, so nothing can preempt
the code in between. The `mux` variable is a lock that matters on two-core
chips. On the one-core C3 it is only a required argument.

**`__attribute__((noinline))`, `__asm__ volatile("")`, `volatile ... sink`.**
An optimising compiler deletes code whose result nobody uses, and a
measurement loop looks exactly like that. These three tell the compiler to
leave the code alone. You will meet them only in `burn_us()` and in the
measuring code of labs 1 and 3.

**`volatile`** tells the compiler that a variable can change behind its back,
in an ISR or another task, so it must really read it every time. It does
**not** make `x = x + 1` safe between tasks: Lab 5 shows exactly that.

**`std::atomic<uint32_t> counter;`** A variable whose operations cannot be
interrupted halfway: `counter.fetch_add(1)` is one indivisible step. Used once,
in Lab 5.

**`printf` formats.** `Serial.printf` works like C's `printf`. Integer types
differ between platforms, so values are cast before printing:
`(unsigned long)x` with `%lu`, `(long)x` with `%ld`, `(long long)x` with
`%lld`.

**`uint32_t`, `int64_t`** are integers of an exact width. Time in microseconds
since boot is `int64_t` (`now_us()`), because 32 bits of microseconds run out
after 71 minutes. The cycle counter is `uint32_t` and wraps every 26.8 s;
subtracting two readings still works across one wrap.

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
