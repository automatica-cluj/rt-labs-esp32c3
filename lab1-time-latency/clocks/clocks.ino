// clocks - what the clocks on the ESP32-C3 can tell you, and what they cost
//
// Runs once after reset, and again on every 'r' in the serial monitor:
//   1. resolution: the smallest step each clock can show
//   2. cost: how long one reading takes, in CPU cycles
//   3. sleeps: how long delay(1), delay(5) and vTaskDelay(1) really last
//   4. busy waits: delayMicroseconds() and burn_us() against the clock
//   5. printing: how long one Serial.printf blocks the caller
//
// Everything is measured first and printed afterwards, except part 5, which
// measures printing on purpose.

#include "rtlab.h"

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
    int64_t a = now_us();
    int64_t b = now_us();
    while (b == a) b = now_us();  // wait for a step, so we start right at one
    a = b;
    while (b == a) b = now_us();  // read until the next step
    return b - a;
}

static void part_resolution(void) {
    Serial.println("\n1. resolution (smallest step you can see)");
    Serial.printf("   millis()             %lu ms\n", (unsigned long)step_millis());
    Serial.printf("   micros()             %lu us\n", (unsigned long)step_micros());
    Serial.printf("   esp_timer_get_time() %lld us\n", (long long)step_esp_timer());
    uint32_t a = cycles(), b = cycles();
    Serial.printf("   cycle counter        %lu cycle(s) between two reads = %.4f us\n",
                  (unsigned long)(b - a), (b - a) / (float)CPU_MHZ);
}

static void part_cost(void) {
    const int kN = 1000;
    volatile uint64_t sink = 0;  // stops the compiler from dropping the calls
    uint32_t c0, c_millis, c_micros, c_timer, c_cycles;

    c0 = cycles();
    for (int i = 0; i < kN; i++) sink += millis();
    c_millis = cycles() - c0;

    c0 = cycles();
    for (int i = 0; i < kN; i++) sink += micros();
    c_micros = cycles() - c0;

    c0 = cycles();
    for (int i = 0; i < kN; i++) sink += now_us();
    c_timer = cycles() - c0;

    c0 = cycles();
    for (int i = 0; i < kN; i++) sink += cycles();
    c_cycles = cycles() - c0;

    Serial.println("\n2. cost of one reading (average of 1000)");
    Serial.printf("   millis()             %4lu cycles = %.2f us\n", (unsigned long)(c_millis / kN), c_millis / (float)kN / CPU_MHZ);
    Serial.printf("   micros()             %4lu cycles = %.2f us\n", (unsigned long)(c_micros / kN), c_micros / (float)kN / CPU_MHZ);
    Serial.printf("   esp_timer_get_time() %4lu cycles = %.2f us\n", (unsigned long)(c_timer / kN), c_timer / (float)kN / CPU_MHZ);
    Serial.printf("   cycle counter        %4lu cycles = %.2f us\n", (unsigned long)(c_cycles / kN), c_cycles / (float)kN / CPU_MHZ);
}

// How long does "sleep for N ms" last? The start is a random point between
// two ticks (we burn a random amount first), as it would be in real code.
static void measure_sleep(const char* what, int kind, int ms) {
    Stats s;
    stats_reset(s, 250);
    for (int i = 0; i < kSleepSamples; i++) {
        burn_us(random(0, 1000));
        int64_t t0 = now_us();
        if (kind == 0)
            delay(ms);
        else
            vTaskDelay(ms);  // in ticks; 1 tick = 1 ms here
        stats_add(s, (int32_t)(now_us() - t0));
    }
    Serial.printf("   %-14s asked %5d us: min %5ld, avg %5ld, max %5ld us\n", what, ms * 1000, (long)s.min,
                  (long)stats_avg(s), (long)s.max);
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
        int64_t t0 = now_us();
        delayMicroseconds(kUs[i]);
        int64_t t1 = now_us();
        burn_us(kUs[i]);
        int64_t t2 = now_us();
        Serial.printf("   %5lu us: delayMicroseconds %6lld us, burn_us %6lld us\n", (unsigned long)kUs[i],
                      (long long)(t1 - t0), (long long)(t2 - t1));
    }
}

static void part_print(void) {
    Serial.printf("\n5. cost of Serial.printf of a 40-character line (%d tries)\n", kPrintSamples);
    int32_t us[kPrintSamples];
    for (int i = 0; i < kPrintSamples; i++) {
        int64_t t0 = now_us();
        Serial.printf("   line %2d: the quick brown fox jumps.\n", i);
        us[i] = (int32_t)(now_us() - t0);
    }
    Stats s;
    stats_reset(s, 100);
    for (int i = 0; i < kPrintSamples; i++) stats_add(s, us[i]);
    stats_print(s, "   printf", "us");
}

static void run_all(void) {
    part_resolution();
    part_cost();
    part_sleep();
    part_busy();
    part_print();
    verdict("CLOCKS_DONE");
    Serial.println("\npress r to run again");
}

void setup() {
    rtlab_begin("lab 1: clocks");
    run_all();
}

void loop() {
    if (read_key() == 'r') run_all();
    delay(20);
}
