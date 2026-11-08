// aperiodic - an aperiodic task that executes text commands
//
// loop() reads lines from the Serial Monitor and puts each line, as text, in
// the command queue, without blocking (timeout 0), the way a network message
// handler would. The queue element holds the text itself (a char array), not
// a pointer: loop() reuses its buffer for the next line.
//
// Each command releases one job of the aperiodic task, which interprets it,
// changes the LED state (through led_set, with the mutex) and answers with
// "OK ..." or "ERROR ...". A wrong command does not stop the task.
//
// A second task, periodic (T = 50 ms), applies the state to the LED.
//
// Commands (Serial Monitor set to "New Line" or "Both NL & CR"), case sensitive:
//   LED ON | LED OFF    the LED lit / off
//   BLINK <ms>          the LED blinks, with half-period <ms>, between 100 and 5000
//   STATUS              the current state, and the verdict line
// Try wrong commands too: BLINK, BLINK 20, BLINK abc, LED, xyz.

// ---- EXPERIMENT KNOBS ----
const uint32_t kLedPeriodMs = 50;
const long kBlinkMinMs = 100;
const long kBlinkMaxMs = 5000;

const int LED_PIN = 8;  // the blue LED: lit when LOW
#define COMMAND_MAX_LEN 32

// int constants, not an enum: the Arduino IDE generates function prototypes
// above the types declared in a sketch, so led_set() cannot take such an enum.
const int MODE_OFF = 0;
const int MODE_ON = 1;
const int MODE_BLINK = 2;
const char* const MODE_NAMES[] = {"OFF", "ON", "BLINK"};

// The state shared by the aperiodic task (writes) and the periodic task
// (reads). It is reached only through led_set() and led_get(), which hold the
// mutex while they write or copy the fields, and no longer.
struct LedState {
    int mode;
    uint32_t blink_ms;
};

static struct LedState g_led = {MODE_OFF, 500};
static SemaphoreHandle_t g_mutex = NULL;

static void led_set(int mode, uint32_t blink_ms) {
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    g_led.mode = mode;
    if (mode == MODE_BLINK) g_led.blink_ms = blink_ms;
    xSemaphoreGive(g_mutex);
}

static void led_get(struct LedState* out) {
    xSemaphoreTake(g_mutex, portMAX_DELAY);
    *out = g_led;
    xSemaphoreGive(g_mutex);
}

struct Command {
    char text[COMMAND_MAX_LEN];
};
static QueueHandle_t g_commands = NULL;
static uint32_t g_ok = 0;     // used only by the aperiodic task
static uint32_t g_error = 0;

static void command_task(void* arg) {
    (void)arg;  // not used
    for (;;) {
        struct Command c;
        xQueueReceive(g_commands, &c, portMAX_DELAY);  // blocked until a command arrives

        if (strcmp(c.text, "LED ON") == 0) {
            led_set(MODE_ON, 0);
            g_ok = g_ok + 1;
            Serial.println("[commands] OK LED ON");
        } else if (strcmp(c.text, "LED OFF") == 0) {
            led_set(MODE_OFF, 0);
            g_ok = g_ok + 1;
            Serial.println("[commands] OK LED OFF");
        } else if (strncmp(c.text, "BLINK ", 6) == 0) {
            long ms = atol(c.text + 6);  // 0 if it is not a number
            if (ms < kBlinkMinMs || ms > kBlinkMaxMs) {
                g_error = g_error + 1;
                Serial.printf("[commands] ERROR BLINK: expected a number between %ld and %ld\n", kBlinkMinMs,
                              kBlinkMaxMs);
            } else {
                led_set(MODE_BLINK, ms);
                g_ok = g_ok + 1;
                Serial.printf("[commands] OK BLINK %ld\n", ms);
            }
        } else if (strcmp(c.text, "STATUS") == 0) {
            struct LedState s;
            led_get(&s);  // the copy: we print without holding the mutex
            g_ok = g_ok + 1;
            Serial.printf("[commands] OK STATUS mode=%s blink=%lu\n", MODE_NAMES[s.mode],
                          (unsigned long)s.blink_ms);
            Serial.printf("VERDICT: LED_MODE=%s BLINK_MS=%lu OK=%lu ERROR=%lu\n", MODE_NAMES[s.mode],
                          (unsigned long)s.blink_ms, (unsigned long)g_ok, (unsigned long)g_error);
        } else {
            g_error = g_error + 1;
            Serial.printf("[commands] ERROR unknown command: %s\n", c.text);
        }
    }
}

// Periodic task (T = kLedPeriodMs): each job copies the state and applies it
// to the LED. Blinking comes from counting jobs: each job adds T, and when the
// sum reaches the half-period blink_ms, the LED toggles.
static void led_task(void* arg) {
    (void)arg;  // not used
    bool on = false;                        // the LED is lit or off now
    uint32_t since_toggle_ms = 0;           // time since the last toggle (BLINK)
    TickType_t last = xTaskGetTickCount();  // the last release, for the grid r_k = r_0 + kT
    for (;;) {
        struct LedState s;
        led_get(&s);  // the copy of the state, taken with the mutex

        since_toggle_ms += kLedPeriodMs;  // one more period has passed
        if (s.mode == MODE_ON) {
            on = true;
        } else if (s.mode == MODE_OFF) {
            on = false;
        } else if (since_toggle_ms >= s.blink_ms) {  // BLINK: the half-period is over
            on = !on;
            since_toggle_ms = 0;
        }
        digitalWrite(LED_PIN, on ? LOW : HIGH);  // the board's LED is lit when LOW
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kLedPeriodMs));  // absolute sleep: no drift
    }
}

// Puts one line in the command queue. It never waits, so it could also be
// called from a network message handler.
static void command_received(const char* text) {
    struct Command c;
    strncpy(c.text, text, sizeof(c.text) - 1);
    c.text[sizeof(c.text) - 1] = '\0';
    if (xQueueSend(g_commands, &c, 0) != pdTRUE) Serial.println("[loop] command queue full");
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 6: aperiodic ===");
    Serial.println("commands: LED ON, LED OFF, BLINK <ms>, STATUS");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED off

    g_mutex = xSemaphoreCreateMutex();
    g_commands = xQueueCreate(5, sizeof(struct Command));

    // function, name, stack in bytes, argument, priority, handle
    xTaskCreate(command_task, "commands", 4096, NULL, 2, NULL);
    xTaskCreate(led_task, "led", 4096, NULL, 3, NULL);
}

void loop() {
    static char line[COMMAND_MAX_LEN];
    static size_t len = 0;
    while (Serial.available() > 0) {
        char ch = Serial.read();
        if (ch == '\n' || ch == '\r') {
            if (len > 0) {
                line[len] = '\0';
                command_received(line);
                len = 0;
            }
        } else if (len < sizeof(line) - 1) {
            line[len++] = ch;
        }
    }
    delay(20);
}
