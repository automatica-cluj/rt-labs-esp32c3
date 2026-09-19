// starvation_wdt - a task that never lets go of the CPU, and the watchdog
//
// loop() runs at priority 1. It blinks the LED and measures the longest gap
// between two of its own iterations. The task watchdog (TWDT) is set to 3 s
// and watches loop(): if loop() does not come round for 3 s, the chip resets.
//
// Keys start a "hog" task at priority 5 that computes without blocking:
//
//   a  hog computes 1 s, then ends          loop() starves for 1 s, no reset
//   b  hog computes 6 s                     loop() starves, TWDT resets the chip
//   c  hog computes 6 s, but sleeps 1 ms    loop() gets the gaps, no reset
//      after every 10 ms of work
//   d  like b, but the hog registers itself with the TWDT and feeds it
//      while it computes
//   i  print the reset report again (USB reconnects after a reset, so the
//      first lines are easy to miss)
//
// After a reset the sketch prints why the chip restarted and which variant was
// running at the time. That survives the reset in RTC memory.

#include "esp_task_wdt.h"
#include "rtlab.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kWdtTimeoutMs = 3000;
const UBaseType_t kHogPrio = 5;

// Ordinary variables are cleared by a reset. These two live in RTC memory,
// which keeps its contents across a software or watchdog reset. That is how
// the sketch knows, after the reset, which variant was running. g_marker holds
// kMagic while a variant runs, to tell a real note from random bits.
const uint32_t kMagic = 0x5741544Bu;
RTC_NOINIT_ATTR uint32_t g_marker;
RTC_NOINIT_ATTR char g_variant;

static SemaphoreHandle_t g_done;
static volatile char g_running = 0;
static int64_t g_last_loop_us = 0;
static int64_t g_max_gap_us = 0;

static const char* reason_name(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON: return "POWERON (power applied)";
        case ESP_RST_SW: return "SW (esp_restart)";
        case ESP_RST_PANIC: return "PANIC (exception or abort)";
        case ESP_RST_INT_WDT: return "INT_WDT (interrupt watchdog)";
        case ESP_RST_TASK_WDT: return "TASK_WDT (task watchdog)";
        case ESP_RST_WDT: return "WDT (other watchdog)";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_USB: return "USB (reset by the USB host)";
        default: return "other";
    }
}

static void hog_task(void*) {
    char v = g_running;  // the variant, set by start_hog()
    uint32_t total_ms = (v == 'a') ? 1000 : 6000;
    if (v == 'd') esp_task_wdt_add(NULL);  // "I am alive, watch me"

    for (uint32_t ms = 0; ms < total_ms; ms += 10) {
        burn_us(10000);
        if (v == 'c') vTaskDelay(1);         // give the CPU away for one tick
        if (v == 'd') esp_task_wdt_reset();  // feed the watchdog
    }

    if (v == 'd') esp_task_wdt_delete(NULL);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void start_hog(char v) {
    Serial.printf("\n--- variant %c: hog at priority %u for %s ---\n", v, (unsigned)kHogPrio,
                  v == 'a' ? "1 s" : "6 s");
    if (v == 'b' || v == 'd')
        Serial.printf("if the watchdog works, the board resets after about %lu ms\n", (unsigned long)kWdtTimeoutMs);
    Serial.flush();  // make sure the text is out before the CPU is taken away

    g_marker = kMagic;  // remembered across a reset
    g_variant = v;
    g_running = v;
    g_max_gap_us = 0;
    g_last_loop_us = now_us();
    start_task(hog_task, "hog", kHogPrio);
}

static void menu(void) {
    Serial.println("\nkeys: a hog 1 s | b hog 6 s | c hog 6 s with sleeps | d hog 6 s feeding the TWDT | i reset report");
}

// What caused the last reset, and which variant was running then.
static esp_reset_reason_t g_reason;
static char g_reset_during = 0;

static void reset_report(void) {
    Serial.printf("reset reason: %s\n", reason_name(g_reason));
    if (g_reset_during) {
        Serial.printf("the reset happened while variant %c was running\n", g_reset_during);
        char v[64];
        snprintf(v, sizeof(v), "RESET_DURING_%c reason=%s", g_reset_during,
                 g_reason == ESP_RST_TASK_WDT ? "TASK_WDT" : "OTHER");
        verdict(v);
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    rtlab_begin("lab 2: starvation_wdt");

    // After power-on, RTC memory holds random bits, so the marker means
    // nothing.
    g_reason = esp_reset_reason();
    if (g_reason != ESP_RST_POWERON && g_marker == kMagic) g_reset_during = g_variant;
    g_marker = 0;
    reset_report();

    // The TWDT is already running (5 s, idle task not watched). Shorten it and
    // make it watch the task that runs loop().
    esp_task_wdt_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.timeout_ms = kWdtTimeoutMs;
    cfg.idle_core_mask = 0;
    cfg.trigger_panic = true;  // reset the chip instead of only printing
    if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) rt_fatal("esp_task_wdt_reconfigure failed");
    enableLoopWDT();  // the Arduino core now feeds the TWDT after every loop()

    g_done = xSemaphoreCreateBinary();
    menu();
}

void loop() {
    int64_t t = now_us();
    if (g_running && t - g_last_loop_us > g_max_gap_us) g_max_gap_us = t - g_last_loop_us;
    g_last_loop_us = t;

    digitalWrite(LED_PIN, (millis() / 250) % 2);  // heartbeat: blinks while loop() runs

    if (g_running && xSemaphoreTake(g_done, 0) == pdTRUE) {
        Serial.printf("hog finished; longest gap between two loop() runs: %.1f ms\n", g_max_gap_us / 1000.0);
        char v[48];
        snprintf(v, sizeof(v), "NO_RESET max_loop_gap_ms=%ld", (long)(g_max_gap_us / 1000));
        verdict(v);
        g_running = 0;
        g_marker = 0;
        menu();
    }

    int k = read_key();
    if (!g_running && k >= 'a' && k <= 'd') start_hog((char)k);
    if (!g_running && k == 'i') reset_report();
    delay(5);
}
