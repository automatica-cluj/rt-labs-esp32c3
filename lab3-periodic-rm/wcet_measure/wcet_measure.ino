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
