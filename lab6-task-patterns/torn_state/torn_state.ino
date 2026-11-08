// torn_state - a state shared by two tasks, with and without a mutex
//
// The state has two fields that must always be equal: x and y.
//
//   writer  priority 2, runs continuously: x = n, some computation, y = n
//   reader  priority 3, periodic (T = 1 ms): copies the state and checks x == y
//
// The reader has the higher priority: when it is released, it preempts the
// writer. Without a mutex it sometimes catches the writer between the two
// writes and copies a half-written state (new x, old y): an inconsistent
// state. With a mutex the reader cannot copy the state while the writer is
// changing it: it blocks in xSemaphoreTake until the writer gives the mutex
// back.
//
// The mutex is held as briefly as possible: only while the fields are written
// or copied. The reader then works on its local copy, without the mutex.
//
//   m  switch the mutex off / on (on at start)
//
// Every 2 s: how many writes, how many reads and how many inconsistent states.
// The first report after each key ends with the verdict line.

// ---- EXPERIMENT KNOBS ----
const uint32_t kWriteGapUs = 50;     // computation between the two writes
const uint32_t kReaderPeriodMs = 1;
const uint32_t kReportMs = 2000;
const UBaseType_t kWriterPrio = 2;
const UBaseType_t kReaderPrio = 3;

struct State {
    uint32_t x;
    uint32_t y;
};

static struct State g_state = {0, 0};
static SemaphoreHandle_t g_mutex = NULL;
static volatile bool g_use_mutex = true;

static volatile uint32_t g_writes = 0;
static volatile uint32_t g_reads = 0;
static volatile uint32_t g_inconsistent = 0;

static void busy_us(uint32_t us) {
    uint32_t start = micros();
    while (micros() - start < us) {
    }
}

static void writer_task(void* arg) {
    (void)arg;  // not used
    uint32_t n = 0;
    for (;;) {
        n = n + 1;
        bool use = g_use_mutex;  // the same decision for take and give
        if (use) xSemaphoreTake(g_mutex, portMAX_DELAY);
        g_state.x = n;
        busy_us(kWriteGapUs);  // between the two writes the state is inconsistent
        g_state.y = n;
        if (use) xSemaphoreGive(g_mutex);
        g_writes = g_writes + 1;

        // Give up the CPU now and then, so that the lower-priority tasks
        // (loop(), the idle task) run too.
        if (n % 20 == 0) vTaskDelay(1);
    }
}

static void reader_task(void* arg) {
    (void)arg;  // not used
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        bool use = g_use_mutex;
        if (use) xSemaphoreTake(g_mutex, portMAX_DELAY);
        struct State copy = g_state;  // inside the mutex we only copy the state...
        if (use) xSemaphoreGive(g_mutex);

        // ...and we work on the copy after giving the mutex back
        g_reads = g_reads + 1;
        if (copy.x != copy.y) g_inconsistent = g_inconsistent + 1;
        xTaskDelayUntil(&last, pdMS_TO_TICKS(kReaderPeriodMs));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 6: torn_state ===");
    Serial.println("key: m = switch the mutex off / on");

    g_mutex = xSemaphoreCreateMutex();  // before the tasks that use it

    // function, name, stack in bytes, argument, priority, handle
    xTaskCreate(writer_task, "writer", 4096, NULL, kWriterPrio, NULL);
    xTaskCreate(reader_task, "reader", 4096, NULL, kReaderPrio, NULL);
}

void loop() {
    static uint32_t last_report = millis();
    static bool verdict_due = false;

    while (Serial.available() > 0) {
        if (Serial.read() == 'm') {
            g_use_mutex = !g_use_mutex;
            Serial.printf("\n--- mutex %s ---\n", g_use_mutex ? "ON" : "OFF");
            // count again, so that the next report is only about the new mode
            last_report = millis();
            g_writes = 0;
            g_reads = 0;
            g_inconsistent = 0;
            verdict_due = true;
        }
    }

    if (millis() - last_report >= kReportMs) {
        last_report = millis();
        uint32_t inconsistent = g_inconsistent;
        Serial.printf("mutex %-3s writes %7lu, reads %5lu, inconsistent states %5lu\n", g_use_mutex ? "ON" : "OFF",
                      (unsigned long)g_writes, (unsigned long)g_reads, (unsigned long)inconsistent);
        if (verdict_due) {
            Serial.printf("VERDICT: MUTEX=%s INCONSISTENT=%lu\n", g_use_mutex ? "ON" : "OFF",
                          (unsigned long)inconsistent);
            verdict_due = false;
        }
        g_writes = 0;
        g_reads = 0;
        g_inconsistent = 0;
    }
    delay(50);
}
