// button_poll - a digital input, read by polling
//
// The BOOT button is on GPIO 9. Pressed, it connects the pin to ground, so
// it reads LOW. Released, nothing drives the pin, so the sketch switches on
// the chip's internal pull-up resistor (INPUT_PULLUP), which makes it read
// HIGH. loop() reads the pin every kPollMs and compares it with the last
// reading: a change from HIGH to LOW is a press, LOW to HIGH a release.
//
//   press the BOOT button: each press and its duration are printed
//   m  switch mode: the LED follows the button / each press toggles the LED
//   r  reset the counter
//   v  print the verdict line
//
// Do not hold BOOT while pressing RST or plugging in: that starts the
// bootloader instead of this sketch.

// ---- EXPERIMENT KNOBS ----
const int kButtonPin = 9;      // BOOT button, LOW when pressed
const int kLedPin = 8;         // blue LED, lit when LOW
const uint32_t kPollMs = 10;   // read the button this often; 0 = on every pass of loop()

const int kModeFollow = 0;     // the LED is lit while the button is held
const int kModeToggle = 1;     // each press switches the LED on or off

static int g_mode = kModeToggle;
static bool g_led_on = false;
static int g_last_level = HIGH;  // released
static uint32_t g_last_poll_ms = 0;
static uint32_t g_pressed_at_ms = 0;
static uint32_t g_presses = 0;

static void led_set(bool on) {
    g_led_on = on;
    digitalWrite(kLedPin, on ? LOW : HIGH);
}

static void poll_button(void) {
    int level = digitalRead(kButtonPin);
    if (level == g_last_level) return;  // nothing changed since the last look
    g_last_level = level;

    uint32_t now = millis();
    if (level == LOW) {  // HIGH -> LOW: pressed
        g_presses = g_presses + 1;
        g_pressed_at_ms = now;
        if (g_mode == kModeToggle) led_set(!g_led_on);
        if (g_mode == kModeFollow) led_set(true);
    } else {  // LOW -> HIGH: released
        if (g_mode == kModeFollow) led_set(false);
        Serial.printf("press %lu, held for %lu ms\n", (unsigned long)g_presses,
                      (unsigned long)(now - g_pressed_at_ms));
    }
}

void setup() {
    pinMode(kLedPin, OUTPUT);
    led_set(false);
    pinMode(kButtonPin, INPUT_PULLUP);
    g_last_level = digitalRead(kButtonPin);

    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 0: button_poll ===");
    Serial.printf("polling every %lu ms; mode: each press toggles the LED\n", (unsigned long)kPollMs);
    Serial.println("press BOOT; keys: m mode | r reset | v verdict");
}

void loop() {
    uint32_t now = millis();
    if (now - g_last_poll_ms >= kPollMs) {
        g_last_poll_ms = now;
        poll_button();
    }

    if (Serial.available() > 0) {
        int k = Serial.read();
        if (k == 'm') {
            g_mode = (g_mode == kModeToggle) ? kModeFollow : kModeToggle;
            led_set(g_mode == kModeFollow && g_last_level == LOW);
            Serial.printf("VERDICT: MODE=%s\n", g_mode == kModeToggle ? "TOGGLE" : "FOLLOW");
        } else if (k == 'r') {
            g_presses = 0;
            Serial.println("counter reset");
        } else if (k == 'v') {
            char v[32];
            snprintf(v, sizeof(v), "PRESSES=%lu", (unsigned long)g_presses);
            Serial.printf("VERDICT: %s\n", v);
        }
    }
}
