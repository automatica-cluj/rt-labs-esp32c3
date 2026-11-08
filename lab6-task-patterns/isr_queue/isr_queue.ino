// isr_queue - the BOOT button, an interrupt routine and a queue
//
// Every falling edge on GPIO9 (the BOOT button, LOW while pressed) runs the
// interrupt routine (ISR). The ISR notes the time and puts one element in the
// queue with xQueueSendFromISR(), nothing else. A task takes the elements from
// the queue and prints them on Serial.
//
// Press BOOT a few times, short and then long. One press often produces
// several interrupts, a few milliseconds apart or even within the same
// millisecond: the contacts bounce. This sketch does not filter them, so that
// you can see them; the filter is in the sporadic sketch.
//
// Rules for an ISR:
//   - it is declared with IRAM_ATTR (the code sits in RAM, not in flash)
//   - it is short: no Serial, no mutex, no waiting
//   - it uses only the FreeRTOS functions with the FromISR suffix
//   - the queue element holds the data itself (here, the time), not a pointer
//
//   press the BOOT button a few times
//   v  print the verdict line with the counts
//
// Do not hold BOOT while pressing RST: that starts the bootloader instead of
// this sketch.

// ---- EXPERIMENT KNOBS ----
const uint32_t kBounceGapMs = 50;       // closer than this to the previous one: probably bounce
const uint32_t kNewPressGapMs = 1000;   // further than this: print it as a new press
const UBaseType_t kQueueLength = 20;

const int BUTTON_PIN = 9;

// One queue element: the time of the interrupt, in ms since start (from ticks).
// A struct, so that fields can be added to it (see the sporadic sketch).
struct Press {
    uint32_t at_ms;
};

static QueueHandle_t g_queue = NULL;
static volatile uint32_t g_interrupts = 0;  // written by the task
static volatile uint32_t g_bounces = 0;

void IRAM_ATTR button_isr(void) {
    struct Press p = {(uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS)};
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(g_queue, &p, &woken);
    portYIELD_FROM_ISR(woken);  // if a higher-priority task was woken, it runs at once
}

static void press_task(void* arg) {
    (void)arg;  // not used
    uint32_t prev_ms = 0;
    for (;;) {
        struct Press p;
        xQueueReceive(g_queue, &p, portMAX_DELAY);  // blocked until an interrupt arrives
        g_interrupts = g_interrupts + 1;
        uint32_t n = g_interrupts;
        uint32_t delta_ms = p.at_ms - prev_ms;
        prev_ms = p.at_ms;

        if (n == 1 || delta_ms > kNewPressGapMs) {  // a new press: a blank line before it
            Serial.printf("\n#%lu  t = %lu ms\n", (unsigned long)n, (unsigned long)p.at_ms);
        } else {
            bool bounce = delta_ms < kBounceGapMs;
            if (bounce) g_bounces = g_bounces + 1;
            Serial.printf("#%lu  t = %lu ms, %lu ms after the previous one%s\n", (unsigned long)n,
                          (unsigned long)p.at_ms, (unsigned long)delta_ms, bounce ? "  <- bounce?" : "");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 6: isr_queue ===");
    Serial.println("press the BOOT button; key: v = verdict line");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    g_queue = xQueueCreate(kQueueLength, sizeof(struct Press));  // the queue, before the ISR and the task

    // function, name, stack in bytes, argument, priority, handle
    xTaskCreate(press_task, "presses", 4096, NULL, 3, NULL);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);  // last
}

void loop() {
    while (Serial.available() > 0) {
        if (Serial.read() == 'v') {
            Serial.printf("VERDICT: INTERRUPTS=%lu BOUNCES=%lu\n", (unsigned long)g_interrupts,
                          (unsigned long)g_bounces);
        }
    }
    delay(20);
}
