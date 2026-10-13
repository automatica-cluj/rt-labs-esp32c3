// two_tasks - two periodic FreeRTOS tasks sharing the serial port
//
//   task "counter"      every 1 s: an integer, a counter
//   task "temperature"  every 2 s: a float, the chip temperature in C
//
// Each task is an endless loop that sleeps until its next release time.
// loop() does nothing: the two tasks do all the work. Both print to the same
// serial port, so a mutex makes sure only one of them prints at a time.
//
// The sketch has no keys. After 10 counter periods it prints one verdict
// line with how many times each task has run.

// ---- EXPERIMENT KNOBS ----
const uint32_t kCounterPeriodMs = 1000;
const uint32_t kTempPeriodMs = 2000;
const UBaseType_t kCounterPrio = 2;  // try 3 for one of them: see the order at t = 0, 2, 4 s
const UBaseType_t kTempPrio = 2;

static SemaphoreHandle_t g_serial_mutex = NULL;
static volatile uint32_t g_temp_runs = 0;
static uint32_t g_t0 = 0;  // millis() when the tasks were started

// Print one finished line. The mutex is held only while printing: the line
// is formatted before, so the other task waits as little as possible.
static void print_line(const char* line) {
    xSemaphoreTake(g_serial_mutex, portMAX_DELAY);
    Serial.print(line);
    xSemaphoreGive(g_serial_mutex);
}

static void counter_task(void* arg) {
    (void)arg;  // not used
    uint32_t n = 0;
    TickType_t last = xTaskGetTickCount();  // the first release is now
    for (;;) {
        n = n + 1;
        char line[80];
        snprintf(line, sizeof(line), "[counter     t=%6lu ms] n = %lu\n", (unsigned long)(millis() - g_t0),
                 (unsigned long)n);
        print_line(line);

        if (n == 10) {
            snprintf(line, sizeof(line), "VERDICT: COUNTER=%lu TEMPERATURE=%lu\n", (unsigned long)n,
                     (unsigned long)g_temp_runs);
            print_line(line);
        }
        // Absolute sleep: wake at last + period, not "period from now", so
        // the time spent printing does not make the period drift.
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kCounterPeriodMs));
    }
}

static void temperature_task(void* arg) {
    (void)arg;  // not used
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        float c = temperatureRead();
        g_temp_runs = g_temp_runs + 1;
        char line[80];
        snprintf(line, sizeof(line), "[temperature t=%6lu ms] chip at %.1f C\n", (unsigned long)(millis() - g_t0), c);
        print_line(line);
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kTempPeriodMs));
    }
}

void setup() {
    Serial.begin(115200);
    // With the board on USB but no Serial Monitor open, each print would
    // wait up to 2 s for a reader (the core retries 20 x 100 ms) and stall
    // this sketch. Timeout 0: text nobody reads is dropped instead.
    Serial.setTxTimeoutMs(0);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 0: two_tasks ===");

    g_serial_mutex = xSemaphoreCreateMutex();

    // Both tasks start now; t= in their lines counts from here.
    g_t0 = millis();
    // function, name, stack in bytes, argument, priority, handle
    xTaskCreate(counter_task, "counter", 4096, NULL, kCounterPrio, NULL);
    xTaskCreate(temperature_task, "temperature", 4096, NULL, kTempPrio, NULL);
}

void loop() {
    delay(1000);  // nothing to do here: the tasks do the work
}
