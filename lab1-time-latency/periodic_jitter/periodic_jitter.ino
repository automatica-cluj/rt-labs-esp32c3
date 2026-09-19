// periodic_jitter - how regular is a periodic task?
//
// One task wants to run every 10 ms and computes for 3 ms each time. It records
// the moment it woke up in every period and prints the numbers when it is done.
// Keys in the serial monitor pick the variant:
//
//   a  relative sleep: vTaskDelay(10) after the work           (priority 5)
//   b  absolute sleep: xTaskDelayUntil(&last, 10)              (priority 5)
//   c  like b, plus a "hog" task at priority 10 that computes
//      2 ms and sleeps 7 ms, over and over                     (priority 5)
//   d  like c, but the periodic task now has priority 15
//   e  like d, but instead of the hog the Wi-Fi radio scans for networks
//   p  print the samples of the last run as CSV (for tools/plot_serial.py)
//
// "lateness" of period k = wake-up time - (start + k * period). A perfect task
// has lateness 0 in every period. jitter = max lateness - min lateness.

#include <WiFi.h>

#include "rtlab.h"

// ---- EXPERIMENT KNOBS ----
const int kPeriodMs = 10;      // T
const int kWorkUs = 3000;      // C, burned with burn_us()
const int kPeriods = 300;      // 3 s per run
const int kHogWorkUs = 2000;   // the hog computes this long ...
const int kHogSleepMs = 7;     // ... then sleeps this long
const UBaseType_t kHogPrio = 10;

static int64_t g_wake[kPeriods];
static bool g_absolute;
static volatile bool g_stop;
static SemaphoreHandle_t g_done;  // every task gives it once when it ends
static int g_ran = 0;             // samples in g_wake from the last run

static void periodic_task(void*) {
    TickType_t last = xTaskGetTickCount();
    for (int k = 0; k < kPeriods; k++) {
        if (g_absolute)
            xTaskDelayUntil(&last, pdMS_TO_TICKS(kPeriodMs));  // wake at last + T
        else
            vTaskDelay(pdMS_TO_TICKS(kPeriodMs));  // wake T after *now*
        g_wake[k] = now_us();
        burn_us(kWorkUs);
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void hog_task(void*) {
    while (!g_stop) {
        burn_us(kHogWorkUs);
        vTaskDelay(pdMS_TO_TICKS(kHogSleepMs));
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// Scanning makes the radio hop channels and send probe requests. The Wi-Fi
// driver does that work in its own task at priority 23 and in interrupts.
static void wifi_task(void*) {
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

    Stats interval, late;
    stats_reset(interval, 1000);
    stats_reset(late, 50);
    for (int k = 0; k < kPeriods; k++) {
        stats_add(late, (int32_t)(g_wake[k] - origin - (int64_t)k * T));
        if (k > 0) stats_add(interval, (int32_t)(g_wake[k] - g_wake[k - 1]));
    }

    stats_print(interval, "interval between wake-ups", "us");
    stats_print(late, "lateness", "us");
    Serial.println("lateness histogram:");
    stats_print_hist(late, "us");

    int64_t took = g_wake[kPeriods - 1] - g_wake[0];
    int64_t should = (int64_t)(kPeriods - 1) * T;
    Serial.printf("drift: %d periods should take %.1f ms, took %.1f ms (%+.1f ms)\n", kPeriods - 1,
                  should / 1000.0, took / 1000.0, (took - should) / 1000.0);

    char v[48];
    if (took - should > T)
        snprintf(v, sizeof(v), "DRIFT_MS=%ld", (long)((took - should) / 1000));
    else
        snprintf(v, sizeof(v), "JITTER_US=%ld", (long)(late.max - late.min));
    verdict(v);
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
    int helpers = 0;
    if (hog) {
        start_task(hog_task, "hog", kHogPrio);
        helpers++;
    }
    if (wifi) {
        WiFi.mode(WIFI_STA);
        start_task(wifi_task, "wifi_load", 1, NULL, 8192);
        helpers++;
        delay(500);  // let the radio start before measuring
    }

    start_task(periodic_task, "periodic", prio);
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
    rtlab_begin("lab 1: periodic_jitter");
    g_done = xSemaphoreCreateCounting(4, 0);
    menu();
}

void loop() {
    int k = read_key();
    if (k >= 'a' && k <= 'e') {
        run((char)k);
        menu();
    } else if (k == 'p') {
        dump_csv();
    }
    delay(20);
}
