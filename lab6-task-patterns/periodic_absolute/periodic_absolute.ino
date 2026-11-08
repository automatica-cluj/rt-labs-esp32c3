// periodic_absolute - one periodic task, with an absolute or a relative sleep
//
// The task has period T = 100 ms. Every job notes its start time s_k and
// compares it with the release grid r_k = r_0 + kT. The difference is the
// lateness L_k = s_k - r_k. Like a real task, each job computes for a varying
// time, here between 5 and 30 ms (simulated by a loop that keeps the CPU busy).
//
//   absolute sleep  xTaskDelayUntil(&last, T)  wakes at r_{k+1}, however long the job took
//   relative sleep  vTaskDelay(T)              wakes T after the end of the job
//
// Every 20 jobs the sketch prints min L, max L and the jitter
// J = max L_k - min L_k. With the absolute sleep, L stays close to 0. With the
// relative sleep, each job adds its computation time to the lateness, and L
// grows from one report to the next: that is drift.
//
//   a  absolute sleep (the default)
//   r  relative sleep
//
// Either key restarts the grid at k = 0, and the first report after it ends
// with the verdict line.

// ---- EXPERIMENT KNOBS ----
const uint32_t kPeriodMs = 100;
const uint32_t kReportEvery = 20;  // jobs in one report
const uint32_t kMinWorkMs = 5;     // the simulated computation time of a job,
const uint32_t kMaxWorkMs = 30;    // between kMinWorkMs and kMaxWorkMs
const int32_t kDriftLimitUs = 50000;  // max L above this counts as drift

// One line of output. The task only fills this in and queues it: loop() does
// the printing, so that printing cannot disturb what is being measured.
struct Report {
    uint32_t run;
    bool absolute;
    uint32_t first_job;
    uint32_t last_job;
    int32_t l_min_us;
    int32_t l_max_us;
};

static QueueHandle_t g_reports = NULL;
static volatile bool g_absolute = true;
static volatile uint32_t g_run = 0;  // loop() adds 1 at every key; the task then restarts the grid

// Simulated computation: keeps the CPU busy for ms milliseconds.
static void busy_ms(uint32_t ms) {
    uint32_t start = micros();
    while (micros() - start < ms * 1000) {
    }
}

static void periodic_task(void* arg) {
    (void)arg;  // not used
    uint32_t run = g_run - 1;  // different from g_run: the first job starts the grid
    bool absolute = true;
    TickType_t last = 0;       // the last release, for xTaskDelayUntil
    uint32_t r0_us = 0;        // r_0, in microseconds
    uint32_t k = 0;            // the job number
    int32_t l_min = INT32_MAX;  // min and max L of the current report
    int32_t l_max = INT32_MIN;

    for (;;) {
        if (run != g_run) {  // a key was pressed: the grid starts again
            run = g_run;
            absolute = g_absolute;
            vTaskDelay(1);  // start the grid exactly at the beginning of a tick
            last = xTaskGetTickCount();
            r0_us = micros();
            k = 0;
            l_min = INT32_MAX;
            l_max = INT32_MIN;
        }

        // s_k - r_k, with r_k = r_0 + kT
        int32_t lateness_us = (int32_t)(micros() - (r0_us + k * kPeriodMs * 1000));
        l_min = min(l_min, lateness_us);
        l_max = max(l_max, lateness_us);
        if ((k + 1) % kReportEvery == 0) {
            struct Report rep = {run, absolute, k + 1 - kReportEvery, k, l_min, l_max};
            xQueueSend(g_reports, &rep, 0);  // never wait: this task has a period to keep
            l_min = INT32_MAX;
            l_max = INT32_MIN;
        }

        busy_ms(random(kMinWorkMs, kMaxWorkMs + 1));  // the job's work
        k = k + 1;

        if (absolute) {
            xTaskDelayUntil(&last, pdMS_TO_TICKS(kPeriodMs));  // wake at r_{k+1}
        } else {
            vTaskDelay(pdMS_TO_TICKS(kPeriodMs));  // wake T from this moment
        }
    }
}

static void print_mode(bool absolute) {
    Serial.printf("\n--- %s sleep (%s), T = %lu ms ---\n", absolute ? "absolute" : "relative",
                  absolute ? "xTaskDelayUntil" : "vTaskDelay", (unsigned long)kPeriodMs);
}

void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 6: periodic_absolute ===");
    Serial.println("keys: a = absolute sleep, r = relative sleep");
    print_mode(true);

    g_reports = xQueueCreate(4, sizeof(struct Report));  // before the task that uses it

    // function, name, stack in bytes, argument, priority, handle
    xTaskCreate(periodic_task, "periodic", 4096, NULL, 2, NULL);
}

void loop() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'a' || c == 'r') {
            g_absolute = (c == 'a');
            g_run = g_run + 1;
            print_mode(c == 'a');
        }
    }

    struct Report rep;
    while (xQueueReceive(g_reports, &rep, 0) == pdTRUE) {
        Serial.printf("jobs %4lu-%4lu: L min %8.3f ms, L max %8.3f ms, jitter J %7.3f ms\n",
                      (unsigned long)rep.first_job, (unsigned long)rep.last_job, rep.l_min_us / 1000.0,
                      rep.l_max_us / 1000.0, (rep.l_max_us - rep.l_min_us) / 1000.0);
        // Only the first report after a key, and not the one after reset.
        if (g_run > 0 && rep.run == g_run && rep.first_job == 0) {
            Serial.printf("VERDICT: %s %s L_MAX_US=%ld\n", rep.absolute ? "ABSOLUTE" : "RELATIVE",
                          rep.l_max_us > kDriftLimitUs ? "DRIFT" : "NO_DRIFT", (long)rep.l_max_us);
        }
    }
    delay(50);
}
