// fsm_periodic - a state machine and two periodic tasks
//
// A parking barrier: it stays closed until a car arrives, then it rises, stays
// up for a while and comes back down.
//
//   CLOSED  --(request)------->  OPENING
//   OPENING --(after 3 s)----->  OPEN      the request is cleared here: the car can pass
//   OPEN    --(after 5 s)----->  CLOSING
//   CLOSING --(after 3 s)----->  CLOSED
//
// Two periodic tasks, with different periods, communicate only through the
// state of the barrier, protected by a mutex:
//
//   control  T = 100 ms   decides the state (the machine above); it prints only
//                         the state changes
//   output   T = 500 ms   decides what is seen: the LED and one line on Serial
//
// What you see: CLOSED = LED off, OPEN = LED lit, OPENING and CLOSING = LED
// blinking. The blinking comes from the output task: lit in one job, off in
// the next, so with the task's period. The output line also shows how many
// seconds of the current state are left, rounded up.
//
// The duration of a state is measured with millis(), a monotonic time: the
// moment the state was entered is kept in the shared state.
//
//   m  a car arrives (the request). A request made while the barrier is up
//      stays recorded and raises the barrier again after it comes down.
//
// Each return to CLOSED prints one verdict line.

// ---- EXPERIMENT KNOBS ----
const uint32_t kControlPeriodMs = 100;
const uint32_t kOutputPeriodMs = 500;
const uint32_t kOpeningMs = 3000;
const uint32_t kOpenMs = 5000;
const uint32_t kClosingMs = 3000;

const int LED_PIN = 8;  // the blue LED: lit when LOW

enum Phase { CLOSED, OPENING, OPEN, CLOSING };
const char* const PHASE_NAMES[] = {"CLOSED", "OPENING", "OPEN", "CLOSING"};
const uint32_t PHASE_MS[] = {0, kOpeningMs, kOpenMs, kClosingMs};  // how long the state lasts; 0 = until an event

struct Barrier {
    enum Phase phase;
    uint32_t phase_start_ms;  // millis() when the current state was entered
    bool request;             // a car has arrived
};

static struct Barrier g_barrier = {CLOSED, 0, false};
static SemaphoreHandle_t g_mutex = NULL;

static void control_task(void* arg) {
    (void)arg;  // not used
    uint32_t cycle_start_ms = 0;  // when the barrier left CLOSED
    uint32_t transitions = 0;     // state changes since then
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        uint32_t now = millis();
        xSemaphoreTake(g_mutex, portMAX_DELAY);
        enum Phase before = g_barrier.phase;
        uint32_t elapsed = now - g_barrier.phase_start_ms;
        enum Phase next = before;
        switch (before) {
            case CLOSED:
                if (g_barrier.request) next = OPENING;
                break;
            case OPENING:
                if (elapsed >= kOpeningMs) next = OPEN;
                break;
            case OPEN:
                if (elapsed >= kOpenMs) next = CLOSING;
                break;
            case CLOSING:
                if (elapsed >= kClosingMs) next = CLOSED;
                break;
        }
        if (next != before) {
            g_barrier.phase = next;
            g_barrier.phase_start_ms = now;
            if (next == OPEN) g_barrier.request = false;
        }
        xSemaphoreGive(g_mutex);

        // Serial only after the mutex is given back
        if (next != before) {
            Serial.printf("[control] %s -> %s\n", PHASE_NAMES[before], PHASE_NAMES[next]);
            if (before == CLOSED) {
                cycle_start_ms = now;
                transitions = 0;
            }
            transitions = transitions + 1;
            if (next == CLOSED) {
                Serial.printf("VERDICT: TRANSITIONS=%lu CYCLE_MS=%lu\n", (unsigned long)transitions,
                              (unsigned long)(now - cycle_start_ms));
            }
        }
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kControlPeriodMs));
    }
}

static void output_task(void* arg) {
    (void)arg;  // not used
    bool blink_on = true;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        xSemaphoreTake(g_mutex, portMAX_DELAY);
        struct Barrier b = g_barrier;  // the copy of the state, with the mutex held as briefly as possible
        xSemaphoreGive(g_mutex);
        uint32_t now = millis();  // after the copy, so never earlier than b.phase_start_ms

        bool led = false;
        switch (b.phase) {
            case CLOSED: led = false; break;
            case OPEN: led = true; break;
            default: led = blink_on; break;  // OPENING, CLOSING: blinking
        }
        blink_on = !blink_on;  // alternates from one job to the next
        digitalWrite(LED_PIN, led ? LOW : HIGH);

        // The seconds left in the current state, rounded up; 0 in CLOSED, which has no duration
        uint32_t duration = PHASE_MS[b.phase];
        uint32_t seconds = 0;
        if (duration > 0) {
            uint32_t elapsed = min(now - b.phase_start_ms, duration);
            seconds = (duration - elapsed + 999) / 1000;
        }
        Serial.printf("[output]  %-7s LED %-3s request %d  left %lu s\n", PHASE_NAMES[b.phase], led ? "on" : "off",
                      b.request ? 1 : 0, (unsigned long)seconds);
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kOutputPeriodMs));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 6: fsm_periodic ===");
    Serial.println("key: m = a car arrives");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED off

    g_mutex = xSemaphoreCreateMutex();
    g_barrier.phase_start_ms = millis();

    // function, name, stack in bytes, argument, priority, handle
    // The control task has the higher priority: its period is shorter.
    xTaskCreate(control_task, "control", 4096, NULL, 3, NULL);
    xTaskCreate(output_task, "output", 4096, NULL, 2, NULL);
}

void loop() {
    while (Serial.available() > 0) {
        if (Serial.read() == 'm') {
            xSemaphoreTake(g_mutex, portMAX_DELAY);
            g_barrier.request = true;
            xSemaphoreGive(g_mutex);
            Serial.println("[loop]    car arrived: request recorded");
        }
    }
    delay(20);
}
