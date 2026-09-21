// periodic_jitter - how regular is a periodic task?
//
// One task wants to run every 10 ms and computes for 3 ms each time. It records
// the moment it woke up in every period and prints the numbers when it is done.
// Keys in the serial monitor pick the variant:
//
//   a  relative sleep: vTaskDelay(10) after the work           (priority 5)
//   b  absolute sleep: xTaskDelayUntil(&last, 10)              (priority 5)
//   c  like b, plus a "hog" task at priority 10, released every
//      9 ms, that computes 2 ms and must be done 4 ms after its
//      release                                                 (priority 5)
//   d  like c, but the periodic task now has priority 15
//   e  like d, but instead of the hog the Wi-Fi radio scans for networks
//   p  print the samples of the last run as CSV (for tools/plot_serial.py)
//
// "lateness" of period k = wake-up time - (start + k * period). A perfect task
// has lateness 0 in every period. jitter = max lateness - min lateness.

#include <WiFi.h>

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const int kPeriodMs = 10;      // T
const int kWorkUs = 3000;      // C, burned with rt_busy_us()
const int kPeriods = 300;      // 3 s per run
const int kHogPeriodMs = 9;        // the hog is released this often ...
const int kHogWorkUs = 2000;       // ... computes this long ...
const int kHogDeadlineUs = 4000;   // ... and must be done this long after its release
const UBaseType_t kHogPrio = 10;
const int kLateUs = 100;           // a period later than this counts as "late"

static int64_t g_wake[kPeriods];
static bool g_absolute;
static volatile bool g_stop;
static SemaphoreHandle_t g_done;  // every task gives it once when it ends
static int g_ran = 0;             // samples in g_wake from the last run
static bool g_hog_on;             // the hog ran in the last run
static struct rt_stats g_hog;     // response time of each hog job
static int g_hog_misses;          // jobs that finished after kHogDeadlineUs

static void periodic_task(void* arg) {
    (void)arg;  // not used
    TickType_t last = xTaskGetTickCount();
    for (int k = 0; k < kPeriods; k++) {
        if (g_absolute)
            xTaskDelayUntil(&last, pdMS_TO_TICKS(kPeriodMs));  // wake at last + T
        else
            vTaskDelay(pdMS_TO_TICKS(kPeriodMs));  // wake T after *now*
        g_wake[k] = rt_now_us();
        rt_busy_us(kWorkUs);
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void hog_task(void* arg) {
    (void)arg;  // not used
    vTaskDelay(1);  // start just after a tick, so the tick count and the clock line up
    TickType_t last = xTaskGetTickCount();
    int64_t release = rt_now_us();  // the release at tick 'last', in microseconds
    while (!g_stop) {
        rt_busy_us(kHogWorkUs);
        // Response time: from the release, not from the moment the hog got the
        // CPU, so time spent waiting for a higher priority task counts too.
        int32_t response = (int32_t)(rt_now_us() - release);
        rt_stats_add(&g_hog, response);
        if (response > kHogDeadlineUs) g_hog_misses++;
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kHogPeriodMs));  // next release: last + 9 ms
        release += kHogPeriodMs * 1000;
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// Scanning makes the radio hop channels and send probe requests. The Wi-Fi
// driver does that work in its own task at priority 23 and in interrupts.
static void wifi_task(void* arg) {
    (void)arg;  // not used
    while (!g_stop) WiFi.scanNetworks(false);
    WiFi.mode(WIFI_OFF);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// Everything in report() is arithmetic on the recorded wake-up times and
// printing. It runs after the measurement, so it cannot disturb it.
static void report(void) {
    const int32_t T = kPeriodMs * 1000;

    // The ideal grid starts at the earliest possible origin, so the least
    // late period has lateness 0.
    int64_t origin = g_wake[0];
    for (int k = 1; k < kPeriods; k++)
        if (g_wake[k] - (int64_t)k * T < origin) origin = g_wake[k] - (int64_t)k * T;

    struct rt_stats interval, late;
    rt_stats_init(&interval, 1000);
    rt_stats_init(&late, 200);  // 12 buckets: 0 .. 2400 us, room for the hog's 2 ms
    int n_late = 0;
    for (int k = 0; k < kPeriods; k++) {
        int32_t l = (int32_t)(g_wake[k] - origin - (int64_t)k * T);
        rt_stats_add(&late, l);
        if (l > kLateUs) n_late++;
        if (k > 0) rt_stats_add(&interval, (int32_t)(g_wake[k] - g_wake[k - 1]));
    }

    rt_stats_print(&interval, "interval between wake-ups", "us");
    rt_stats_print(&late, "lateness", "us");
    Serial.printf("late by more than %d us: %d of %d periods\n", kLateUs, n_late, kPeriods);
    Serial.println("lateness histogram:");
    rt_stats_print_hist(&late, "us");

    int64_t took = g_wake[kPeriods - 1] - g_wake[0];
    int64_t should = (int64_t)(kPeriods - 1) * T;
    Serial.printf("drift: %d periods should take %.1f ms, took %.1f ms (%+.1f ms)\n", kPeriods - 1,
                  should / 1000.0, took / 1000.0, (took - should) / 1000.0);

    if (g_hog_on) {
        Serial.printf("hog: %lu jobs of %d us, response min %ld, avg %ld, max %ld us, %d missed the %d us deadline\n",
                      (unsigned long)g_hog.n, kHogWorkUs, (long)g_hog.min, (long)rt_stats_avg(&g_hog),
                      (long)g_hog.max, g_hog_misses, kHogDeadlineUs);
    }

    char v[48];
    if (took - should > T)
        snprintf(v, sizeof(v), "DRIFT_MS=%ld", (long)((took - should) / 1000));
    else
        snprintf(v, sizeof(v), "JITTER_US=%ld", (long)(late.max - late.min));
    rt_verdict(v);
}

static void run(char key) {
    UBaseType_t prio = (key == 'd' || key == 'e') ? 15 : 5;
    bool hog = (key == 'c' || key == 'd');
    bool wifi = (key == 'e');
    g_absolute = (key != 'a');

    const char* other = "nothing else running";
    if (hog) other = "hog at priority 10";
    if (wifi) other = "Wi-Fi scanning";
    Serial.printf("\n--- variant %c: %s sleep, periodic task priority %u, %s ---\n", key,
                  g_absolute ? "absolute" : "relative", (unsigned)prio, other);
    Serial.printf("period %d ms, work %d us, %d periods\n", kPeriodMs, kWorkUs, kPeriods);

    g_stop = false;
    g_hog_on = hog;
    rt_stats_init(&g_hog, 500);
    g_hog_misses = 0;
    int helpers = 0;
    if (hog) {
        rt_start_task(hog_task, "hog", kHogPrio, NULL);
        helpers++;
    }
    if (wifi) {
        WiFi.mode(WIFI_STA);
        rt_start_task_stack(wifi_task, "wifi_load", 1, NULL, 8192);
        helpers++;
        delay(500);  // let the radio start before measuring
    }

    rt_start_task(periodic_task, "periodic", prio, NULL);
    xSemaphoreTake(g_done, portMAX_DELAY);  // the periodic task is done
    g_stop = true;
    for (int i = 0; i < helpers; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    g_ran = kPeriods;

    report();  // printing starts only now that nothing is being measured
}

static void dump_csv(void) {
    if (g_ran == 0) {
        Serial.println("no run yet");
        return;
    }
    Serial.println("csv: k,wake_us,interval_us");
    for (int k = 0; k < g_ran; k++)
        Serial.printf("%d,%lld,%lld\n", k, (long long)(g_wake[k] - g_wake[0]),
                      (long long)(k ? g_wake[k] - g_wake[k - 1] : 0));
    Serial.println("csv: end");
}

static void menu(void) {
    Serial.println("\nkeys: a relative | b absolute | c + hog | d + hog, high priority | e + Wi-Fi | p CSV");
}

void setup() {
    rt_begin("lab 1: periodic_jitter");
    g_done = xSemaphoreCreateCounting(4, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'e') {
        run((char)k);
        menu();
    } else if (k == 'p') {
        dump_csv();
    }
    delay(20);
}
