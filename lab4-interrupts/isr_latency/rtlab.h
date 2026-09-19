// rtlab.h - the small toolbox shared by every lab sketch
//
// This is the master copy. tools/sync_common.sh copies it next to every .ino
// file, because the Arduino IDE only finds headers in the sketch folder. Edit
// it here, never in a lab folder.
//
// Read it once; every function is one idea:
//
//   rtlab_begin()     open USB serial, calibrate burn_us(), print a banner
//   cycles()          CPU cycle counter, 160 per microsecond
//   now_us()          microseconds since boot (esp_timer)
//   burn_us()         spend N microseconds of CPU time: the "computation"
//   start_task()      xTaskCreate that stops the sketch if it fails
//   Stats             min / avg / max and a histogram of integer samples
//   Timeline          which task had the CPU in each slice of time
//   read_key()        one key from the serial monitor, or -1
//   verdict()         the one line a lab run is judged by
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
static inline uint32_t cycles(void) { return esp_cpu_get_cycle_count(); }
static inline uint32_t cycles_to_us(uint32_t c) { return c / CPU_MHZ; }
static inline int64_t now_us(void) { return esp_timer_get_time(); }

// --------------------------------------------------------- busy work ------

// burn_us(n) runs a fixed number of loop iterations. Its length is CPU time,
// not wall-clock time: if the task is preempted halfway, the rest of the work
// still has to be done afterwards. That is how real computation behaves, and
// it is what makes the response times in labs 2, 3 and 5 come out right.
static uint32_t g_loops_per_ms = 0;

// An optimising compiler removes loops that compute nothing. Two things stop
// it here: noinline keeps spin_loops() a real function call, and the empty
// asm statement counts as "an effect", so every iteration has to stay.
static void __attribute__((noinline)) spin_loops(uint32_t n) {
    for (uint32_t i = 0; i < n; i++) __asm__ volatile("");
}

static inline void burn_us(uint32_t us) {
    // Whole milliseconds first, then the rest, so the product cannot overflow.
    uint32_t ms = us / 1000;
    for (uint32_t i = 0; i < ms; i++) spin_loops(g_loops_per_ms);
    spin_loops((us % 1000) * g_loops_per_ms / 1000);
}

// Measure how many loop iterations fit in one millisecond. Interrupts are
// off during each try, so nothing else is counted. The fastest of five tries
// is the one nobody disturbed.
static inline void calibrate_burn(void) {
    // portENTER_CRITICAL wants a lock variable. On this one-core chip the
    // lock does nothing extra: "critical" simply means interrupts are off.
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    const uint32_t kTrial = 20000;
    uint32_t best = 0xFFFFFFFFu;
    for (int t = 0; t < 5; t++) {
        portENTER_CRITICAL(&mux);
        uint32_t c0 = cycles();
        spin_loops(kTrial);
        uint32_t c = cycles() - c0;
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
// arg and stack have default values: start_task(fn, "name", 3) is enough.
static inline TaskHandle_t start_task(TaskFunction_t fn, const char* name, UBaseType_t prio,
                               void* arg = NULL, uint32_t stack = 4096) {
    TaskHandle_t h = NULL;
    if (xTaskCreate(fn, name, stack, arg, prio, &h) != pdPASS) rt_fatal("xTaskCreate failed");
    return h;
}

// ------------------------------------------------------------- stats ------

#define STATS_BUCKETS 12

struct Stats {
    uint32_t n;
    int32_t min, max;
    int64_t sum;
    int32_t bucket;              // width of one histogram bucket
    uint32_t hist[STATS_BUCKETS];  // the last bucket collects everything above
};

static inline void stats_reset(Stats& s, int32_t bucket) {
    memset(&s, 0, sizeof(s));
    s.min = INT32_MAX;
    s.max = INT32_MIN;
    s.bucket = bucket > 0 ? bucket : 1;
}

static inline void stats_add(Stats& s, int32_t v) {
    s.n++;
    s.sum += v;
    if (v < s.min) s.min = v;
    if (v > s.max) s.max = v;
    int32_t b = v < 0 ? 0 : v / s.bucket;
    if (b >= STATS_BUCKETS) b = STATS_BUCKETS - 1;
    s.hist[b]++;
}

static inline int32_t stats_avg(const Stats& s) { return s.n ? (int32_t)(s.sum / (int64_t)s.n) : 0; }

static inline void stats_print(const Stats& s, const char* what, const char* unit) {
    if (s.n == 0) {
        Serial.printf("%s: no samples\n", what);
        return;
    }
    Serial.printf("%s over %lu samples: min %ld %s, avg %ld %s, max %ld %s\n", what,
                  (unsigned long)s.n, (long)s.min, unit, (long)stats_avg(s), unit, (long)s.max, unit);
}

static inline void stats_print_hist(const Stats& s, const char* unit) {
    for (int b = 0; b < STATS_BUCKETS; b++) {
        long lo = (long)b * s.bucket;
        if (b < STATS_BUCKETS - 1)
            Serial.printf("  %7ld .. %7ld %-3s %7lu\n", lo, lo + s.bucket, unit, (unsigned long)s.hist[b]);
        else
            Serial.printf("  %7ld ..     and up %7lu\n", lo, (unsigned long)s.hist[b]);
    }
}

// ---------------------------------------------------------- timeline ------

// A strip with one character per slice of time. A task that is running writes
// its letter into the slice for "now"; slices nobody wrote stay '.'. Printing
// the strip afterwards shows who had the CPU and when.
#define TL_MAX 400

struct Timeline {
    int64_t t0;
    int32_t slice_us;
    int n;
    char c[TL_MAX];
};

static Timeline g_tl;

static inline void tl_start(int32_t slice_us, int n) {
    g_tl.slice_us = slice_us;
    g_tl.n = n < TL_MAX ? n : TL_MAX;
    memset(g_tl.c, '.', sizeof(g_tl.c));
    g_tl.t0 = now_us();
}

static inline void tl_mark(char who) {
    int64_t i = (now_us() - g_tl.t0) / g_tl.slice_us;
    if (i >= 0 && i < g_tl.n) g_tl.c[i] = who;
}

// burn_us() that also marks the timeline every 100 us of work.
static inline void tl_burn(uint32_t us, char who) {
    for (uint32_t done = 0; done < us; done += 100) {
        tl_mark(who);
        burn_us(us - done < 100 ? us - done : 100);
    }
    tl_mark(who);
}

// Milliseconds since tl_start(), for event logs printed next to the strip.
static inline float tl_ms(void) { return (now_us() - g_tl.t0) / 1000.0f; }

static inline void tl_print(int per_row) {
    Serial.printf("timeline, one column = %ld us ('.' = nobody marked it)\n", (long)g_tl.slice_us);
    for (int row = 0; row < g_tl.n; row += per_row) {
        Serial.printf("%6ld ms |", (long)((int64_t)row * g_tl.slice_us / 1000));
        for (int i = row; i < row + per_row && i < g_tl.n; i++) Serial.write(g_tl.c[i]);
        Serial.println("|");
    }
}

// ------------------------------------------------------------ serial ------

static inline int read_key(void) {
    if (!Serial.available()) return -1;
    int k = Serial.read();
    return (k == '\r' || k == '\n' || k == ' ') ? -1 : k;
}

static inline void verdict(const char* v) { Serial.printf("VERDICT: %s\n", v); }

// Call first in setup(). Waits up to 3 s for the serial monitor so the first
// lines are not lost, then calibrates busy work at the highest lab priority.
static inline void rtlab_begin(const char* title) {
    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);

    UBaseType_t old = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 20);
    calibrate_burn();
    vTaskPrioritySet(NULL, old);

    Serial.printf("\n=== %s ===\n", title);
    Serial.printf("ESP32-C3 at %d MHz, tick %d Hz, burn_us: %lu loops per ms\n", CPU_MHZ,
                  configTICK_RATE_HZ, (unsigned long)g_loops_per_ms);
}
