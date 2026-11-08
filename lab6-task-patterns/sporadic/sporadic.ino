// sporadic - a sporadic task with a minimum inter-arrival time
//
// Presses come from two sources, into the same queue:
//   - the BOOT button, through the interrupt routine (as in isr_queue)
//   - the key b in the Serial Monitor, queued from loop() without blocking
//     (timeout 0), the way a network message handler would queue a command
//
// The sporadic task takes the presses and accepts one only if at least the
// minimum inter-arrival time (kMinInterarrivalMs) has passed since the last
// accepted press. The others are rejected, with a message. This also removes
// the extra interrupts caused by bounce, without any filter in the ISR. At
// each accepted press the board's LED changes state.
//
// The arrival time is taken when the press enters the queue (in the ISR or in
// loop()) and travels inside the queue element. If the task read the clock
// when it takes the element, it would measure when it got round to handling
// it, not when it arrived.
//
//   press BOOT rarely, then often
//   b  one press from the keyboard; bbbb + Enter is four presses at once
//   v  print the verdict line with the counts
//
// Do not hold BOOT while pressing RST: that starts the bootloader instead of
// this sketch.

// ---- EXPERIMENT KNOBS ----
const uint32_t kMinInterarrivalMs = 1000;  // the minimum inter-arrival time

const int BUTTON_PIN = 9;
const int LED_PIN = 8;  // the blue LED: lit when LOW

struct Press {
    uint32_t at_ms;   // the arrival time, ms since start
    bool from_board;  // true = BOOT (ISR), false = key b
};

static QueueHandle_t g_queue = NULL;
static volatile uint32_t g_accepted = 0;  // written by the task
static volatile uint32_t g_rejected = 0;

void IRAM_ATTR button_isr(void) {
    struct Press p = {(uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS), true};
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(g_queue, &p, &woken);
    portYIELD_FROM_ISR(woken);
}

static void sporadic_task(void* arg) {
    (void)arg;  // not used
    bool any = false;      // has a press been accepted yet?
    uint32_t last_ms = 0;  // the time of the last accepted press
    bool led_on = false;
    for (;;) {
        struct Press p;
        xQueueReceive(g_queue, &p, portMAX_DELAY);  // a press releases a job
        const char* src = p.from_board ? "BOOT" : "key b";
        uint32_t since = p.at_ms - last_ms;

        if (any && since < kMinInterarrivalMs) {
            g_rejected = g_rejected + 1;
            Serial.printf("[sporadic] %-5s t = %6lu ms  REJECTED: %lu ms after the accepted one\n", src,
                          (unsigned long)p.at_ms, (unsigned long)since);
            continue;
        }
        any = true;
        last_ms = p.at_ms;
        g_accepted = g_accepted + 1;
        led_on = !led_on;
        digitalWrite(LED_PIN, led_on ? LOW : HIGH);
        Serial.printf("[sporadic] %-5s t = %6lu ms  ACCEPTED, LED %s\n", src, (unsigned long)p.at_ms,
                      led_on ? "on" : "off");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 6: sporadic ===");
    Serial.printf("press BOOT or type b; v = verdict line; minimum inter-arrival time %lu ms\n",
                  (unsigned long)kMinInterarrivalMs);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED off
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    g_queue = xQueueCreate(10, sizeof(struct Press));

    // function, name, stack in bytes, argument, priority, handle
    xTaskCreate(sporadic_task, "sporadic", 4096, NULL, 3, NULL);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);
}

void loop() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'b') {
            struct Press p = {(uint32_t)millis(), false};
            if (xQueueSend(g_queue, &p, 0) != pdTRUE) Serial.println("[loop] queue full, press lost");
        } else if (c == 'v') {
            Serial.printf("VERDICT: ACCEPTED=%lu REJECTED=%lu\n", (unsigned long)g_accepted,
                          (unsigned long)g_rejected);
        }
    }
    delay(20);
}
