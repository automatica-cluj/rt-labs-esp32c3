// pwm_fade - brightness from an on/off pin: pulse-width modulation (PWM)
//
// A digital pin is either on or off. To make the LED half as bright, switch
// it on and off fast, so that it is on half of the time. The fraction of time
// it is on is the duty cycle. Done slowly, you see the blinking; done fast
// enough, the eye averages it into a dimmer light.
//
// Keys s and h do PWM by hand, with digitalWrite() and a delay. The other
// keys use analogWrite(), which hands the job to a hardware peripheral (LEDC)
// that keeps switching the pin while the CPU does something else.
//
//   0..9  brightness with analogWrite(): 0 = off ... 9 = full
//   f     fade up and down once with analogWrite()
//   s     PWM by hand at kSlowHz for 3 s: you see it blink
//   h     PWM by hand at kFastHz for 3 s: you see it dim

// ---- EXPERIMENT KNOBS ----
const int kLedPin = 8;            // blue LED, lit when LOW
const uint32_t kSlowHz = 5;       // PWM by hand, slow
const uint32_t kFastHz = 500;     // PWM by hand, fast
const int kHandDutyPercent = 25;  // duty cycle for keys s and h
const uint32_t kFadeMs = 2000;    // one fade, up and down

// The brightness for each digit key, in percent. The steps are small at the
// bottom on purpose: the eye notices a change from 1 % to 2 % much more than
// one from 75 % to 76 %.
static const int kLevels[10] = {0, 1, 2, 5, 10, 20, 35, 50, 75, 100};

// The PWM period has 256 steps, and analogWrite(pin, v) keeps the pin HIGH
// for v of them (255 and above means HIGH all the time). This LED lights when
// the pin is LOW, so to light it for `lit` steps out of 256, write 256 - lit.
// lit = 0 is off, lit = 256 is full on.
static int led_brightness(int lit) {
    int v = 256 - lit;
    analogWrite(kLedPin, v);
    return v;
}

static void pwm_by_hand(uint32_t hz) {
    uint32_t period_us = 1000000 / hz;
    uint32_t on_us = period_us * kHandDutyPercent / 100;
    uint32_t off_us = period_us - on_us;
    Serial.printf("by hand: %lu Hz, period %lu us, LED on %lu us, off %lu us, for 3 s\n", (unsigned long)hz,
                  (unsigned long)period_us, (unsigned long)on_us, (unsigned long)off_us);

    pinMode(kLedPin, OUTPUT);  // take the pin back from the PWM peripheral
    uint32_t periods = 3 * hz;
    for (uint32_t i = 0; i < periods; i++) {
        digitalWrite(kLedPin, LOW);  // on
        delayMicroseconds(on_us);
        digitalWrite(kLedPin, HIGH);  // off
        delayMicroseconds(off_us);
    }
    char v[32];
    snprintf(v, sizeof(v), "SOFT_PWM_HZ=%lu", (unsigned long)hz);
    Serial.printf("VERDICT: %s\n", v);
}

static void fade(void) {
    Serial.printf("fading up and down in %lu ms\n", (unsigned long)kFadeMs);
    uint32_t step_ms = kFadeMs / (2 * 257);
    if (step_ms == 0) step_ms = 1;
    for (int b = 0; b <= 256; b++) {
        led_brightness(b);
        delay(step_ms);
    }
    for (int b = 256; b >= 0; b--) {
        led_brightness(b);
        delay(step_ms);
    }
    Serial.println("VERDICT: FADE_DONE");
}

void setup() {
    led_brightness(0);

    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 0: pwm_fade ===");
    Serial.println("keys: 0..9 brightness | f fade | s slow PWM by hand | h fast PWM by hand");
}

void loop() {
    if (Serial.available() > 0) {
        int k = Serial.read();
        if (k >= '0' && k <= '9') {
            int percent = kLevels[k - '0'];
            int lit = percent * 256 / 100;
            int value = led_brightness(lit);
            Serial.printf("brightness %d %%: lit %d of 256 steps, analogWrite(%d, %d)\n", percent, lit, kLedPin,
                          value);
            char v[32];
            snprintf(v, sizeof(v), "DUTY_PERCENT=%d", percent);
            Serial.printf("VERDICT: %s\n", v);
        } else if (k == 'f') {
            fade();
        } else if (k == 's') {
            pwm_by_hand(kSlowHz);
        } else if (k == 'h') {
            pwm_by_hand(kFastHz);
        }
    }
    delay(10);
}
