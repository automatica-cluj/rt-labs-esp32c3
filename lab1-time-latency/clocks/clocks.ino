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
