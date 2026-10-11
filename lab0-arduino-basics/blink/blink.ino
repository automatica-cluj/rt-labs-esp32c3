// blink - a digital output, and two ways to wait
//
// The blue LED sits on GPIO 8 and is lit when the pin is LOW. The sketch
// blinks it with millis(): loop() never waits, it only checks whether it is
// time to toggle. So loop() stays free to read keys at the same time.
// Key a blinks the other way, with delay(), to show what that costs.
//
//   a  blink 5 times with delay(): loop() is stuck until it is done
//   b  turn the millis() blinking on or off
//   +  blink twice as fast       -  blink twice as slow
//   l  count how many times loop() runs in one second

// ---- EXPERIMENT KNOBS ----
const int kLedPin = 8;            // the blue LED on the Super Mini
const uint32_t kStartHalfMs = 500;  // LED on for this long, then off for this long
const int kBlockingBlinks = 5;    // how many blinks key a does with delay()

static uint32_t g_half_ms = kStartHalfMs;
static bool g_blinking = true;
static bool g_led_on = false;
static uint32_t g_last_toggle_ms = 0;

// Counting loop() passes for key l
static bool g_counting = false;
static uint32_t g_count = 0;
static uint32_t g_count_start_ms = 0;

static void led_set(bool on) {
    g_led_on = on;
    digitalWrite(kLedPin, on ? LOW : HIGH);  // LOW lights this LED
}

static void blink_with_delay(void) {
    Serial.printf("blinking %d times with delay(%lu); type some keys meanwhile...\n", kBlockingBlinks,
                  (unsigned long)g_half_ms);
    uint32_t t0 = millis();
    for (int i = 0; i < kBlockingBlinks; i++) {
        led_set(true);
        delay(g_half_ms);
        led_set(false);
        delay(g_half_ms);
    }
    uint32_t blocked = millis() - t0;

    Serial.printf("loop() was stuck for %lu ms; %d characters waited in the input buffer\n",
                  (unsigned long)blocked, Serial.available());
    while (Serial.available() > 0) Serial.read();  // drop them, they came too late

    g_last_toggle_ms = millis();
    char v[48];
    snprintf(v, sizeof(v), "BLOCKED_MS=%lu", (unsigned long)blocked);
    Serial.printf("VERDICT: %s\n", v);
}

void setup() {
    pinMode(kLedPin, OUTPUT);
    led_set(false);

    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 0: blink ===");
    Serial.println("keys: a blink with delay() | b blinking on/off | + faster | - slower | l loop() rate");
    g_last_toggle_ms = millis();
}

void loop() {
    uint32_t now = millis();

    // The non-blocking blink: toggle only when half a period has passed.
    // now - last stays correct even when millis() wraps after 49.7 days.
    if (g_blinking && now - g_last_toggle_ms >= g_half_ms) {
        g_last_toggle_ms = g_last_toggle_ms + g_half_ms;  // keeps the rhythm exact
        led_set(!g_led_on);
    }

    if (g_counting) {
        g_count = g_count + 1;
        if (now - g_count_start_ms >= 1000) {
            g_counting = false;
            Serial.printf("loop() ran %lu times in one second, while the LED kept blinking\n",
                          (unsigned long)g_count);
            char v[48];
            snprintf(v, sizeof(v), "LOOPS_PER_S=%lu", (unsigned long)g_count);
            Serial.printf("VERDICT: %s\n", v);
        }
    }

    if (Serial.available() > 0) {
        int k = Serial.read();
        if (k == 'a') {
            blink_with_delay();
        } else if (k == 'b') {
            g_blinking = !g_blinking;
            if (!g_blinking) led_set(false);
            g_last_toggle_ms = millis();
            Serial.printf("VERDICT: BLINK=%s\n", g_blinking ? "ON" : "OFF");
        } else if (k == '+' && g_half_ms > 25) {
            g_half_ms = g_half_ms / 2;
            Serial.printf("LED on %lu ms, off %lu ms\n", (unsigned long)g_half_ms, (unsigned long)g_half_ms);
        } else if (k == '-' && g_half_ms < 4000) {
            g_half_ms = g_half_ms * 2;
            Serial.printf("LED on %lu ms, off %lu ms\n", (unsigned long)g_half_ms, (unsigned long)g_half_ms);
        } else if (k == 'l') {
            g_count = 0;
            g_count_start_ms = millis();
            g_counting = true;
        }
    }
    // No delay() here: loop() comes back as fast as it can.
}
