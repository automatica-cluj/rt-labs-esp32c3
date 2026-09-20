// button - a real input: the BOOT button, its interrupt and its bounce
//
// A mechanical contact does not switch cleanly. For a few hundred
// microseconds it closes and opens many times ("bounce"), and every one of
// those edges is an interrupt.
//
// The ISR does as little as possible: it counts the edge, notes the time and
// wakes a task. The task does the rest (deferred processing): it waits until
// the contact has settled, reads the pin once, and only then decides whether
// the button was pressed. Each debounced press toggles the LED.
//
//   press the BOOT button a few times, then look at the counters
//   r  reset the counters
//   v  print the verdict line
//
// Do not hold BOOT while pressing RESET or plugging in: that starts the
// bootloader instead of this sketch.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kSettleMs = 20;  // how long the contact must be quiet

// Written by the ISR
static volatile uint32_t g_edges = 0;
static volatile uint32_t g_last_edge_cyc = 0;
static volatile uint32_t g_min_gap_cyc = 0xFFFFFFFFu;  // shortest time between two edges
// Written by the debounce task
static volatile uint32_t g_presses = 0;
static volatile uint32_t g_releases = 0;

static TaskHandle_t g_debounce = NULL;

static void ARDUINO_ISR_ATTR on_edge(void) {
    uint32_t c = rt_cycles();
    if (g_edges > 0 && c - g_last_edge_cyc < g_min_gap_cyc) g_min_gap_cyc = c - g_last_edge_cyc;
    g_last_edge_cyc = c;
    g_edges = g_edges + 1;

    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(g_debounce, &woken);
    portYIELD_FROM_ISR(woken);
}

static void debounce_task(void* arg) {
    (void)arg;  // not used
    int stable = digitalRead(BOOT_PIN);  // HIGH = released
    bool led_on = false;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // some edge happened
        // Wait until no new edge has arrived for kSettleMs.
        while (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kSettleMs)) > 0) {
        }
        int level = digitalRead(BOOT_PIN);
        if (level == stable) continue;  // bounced and came back: no change
        stable = level;
        if (level == LOW) {
            g_presses = g_presses + 1;
            led_on = !led_on;
            digitalWrite(LED_PIN, led_on ? LOW : HIGH);  // the LED is lit when LOW
        } else {
            g_releases = g_releases + 1;
        }
    }
}

static void print_counters(void) {
    Serial.printf("edges %4lu | debounced presses %3lu, releases %3lu | edges per press+release %.1f | "
                  "shortest gap between edges %s",
                  (unsigned long)g_edges, (unsigned long)g_presses, (unsigned long)g_releases,
                  (g_presses + g_releases) ? g_edges / (float)(g_presses + g_releases) : 0.0f,
                  g_min_gap_cyc == 0xFFFFFFFFu ? "-" : "");
    if (g_min_gap_cyc != 0xFFFFFFFFu) Serial.printf("%.1f us", g_min_gap_cyc / (float)CPU_MHZ);
    Serial.println();
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // off
    pinMode(BOOT_PIN, INPUT_PULLUP);
    rt_begin("lab 4: button");

    g_debounce = rt_start_task(debounce_task, "debounce", 5, NULL);
    attachInterrupt(BOOT_PIN, on_edge, CHANGE);
    Serial.println("press the BOOT button; keys: r reset counters | v verdict");
}

void loop() {
    static uint32_t shown_edges = 0, shown_presses = 0, shown_releases = 0;
    // Print only when something changed, and only after the button has been
    // quiet for a moment, so printing never competes with an ongoing bounce.
    if ((g_edges != shown_edges || g_presses != shown_presses || g_releases != shown_releases) &&
        rt_cycles() - g_last_edge_cyc > 100u * 1000 * CPU_MHZ) {
        shown_edges = g_edges;
        shown_presses = g_presses;
        shown_releases = g_releases;
        print_counters();
    }

    int k = rt_read_key();
    if (k == 'r') {
        noInterrupts();
        g_edges = 0;
        g_presses = 0;
        g_releases = 0;
        g_min_gap_cyc = 0xFFFFFFFFu;
        interrupts();
        shown_edges = shown_presses = shown_releases = 0;
        Serial.println("counters reset");
    } else if (k == 'v') {
        char v[64];
        snprintf(v, sizeof(v), "EDGES=%lu PRESSES=%lu", (unsigned long)g_edges, (unsigned long)g_presses);
        rt_verdict(v);
    }
    delay(10);
}
