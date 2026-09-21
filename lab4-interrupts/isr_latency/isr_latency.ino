// isr_latency - from a hardware event to the ISR, and from the ISR to a task
//
// A hardware timer counts at 10 MHz and raises an alarm every 1 ms. At the
// alarm it restarts from 0, so the value the ISR reads on entry is exactly how
// long the ISR took to start: the interrupt latency (0.1 us resolution).
// The ISR then wakes a handler task with a task notification. The handler
// measures how long after the ISR it got to run: the ISR-to-task latency.
//
//   a  handler priority 10, ISR asks for a context switch (portYIELD_FROM_ISR)
//   b  like a, but the ISR does not ask for the switch
//   c  handler priority 2, while a task at priority 5 computes 3 ms, sleeps 2 ms
//   d  like a, while a task at priority 3 keeps disabling interrupts for
//      200 us at a time (a critical section)
//   e  like a, while a second timer's ISR does 300 us of work every 3.7 ms
//   f  like e, but that ISR only wakes a task (priority 5) that does the work
//
// Nothing is printed while measuring. The ISR and the handler store numbers in
// arrays; loop() prints them afterwards.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const int kSamples = 2000;          // 1 ms apart: 2 s per run
const uint32_t kTimerHz = 10000000; // 10 MHz, 0.1 us per count
const uint32_t kAlarmCounts = 10000;  // 1 ms
const uint32_t kCriticalUs = 200;   // variant d
const uint32_t kNoisyWorkUs = 300;  // variants e and f
const uint32_t kNoisyPeriodUs = 3700;

static hw_timer_t* g_timer = NULL;  // the timer we measure
static hw_timer_t* g_noisy = NULL;  // the other one, variants e and f
static TaskHandle_t g_handler = NULL;
static TaskHandle_t g_worker = NULL;

static volatile bool g_yield;
static volatile bool g_noisy_defers;
static volatile bool g_stop;
static SemaphoreHandle_t g_done;

// Written by the ISR
static uint16_t g_isr_counts[kSamples];  // timer counts at ISR entry
static uint32_t g_isr_cycles[kSamples];  // cycle counter at ISR entry
static volatile int g_isr_n;
// Written by the handler task
static uint32_t g_task_cycles[kSamples];  // ISR entry -> handler running
static volatile int g_task_n;
static int g_consumed;                    // events the handler has dealt with
static volatile uint32_t g_merged;        // events that shared a wake-up

// ------------------------------------------------------------ ISRs --------

static void ARDUINO_ISR_ATTR on_alarm(void) {
    uint32_t counts = (uint32_t)timerRead(g_timer);  // first thing: latency
    uint32_t c = rt_cycles();
    int n = g_isr_n;
    if (n >= kSamples) return;
    g_isr_counts[n] = (uint16_t)counts;
    g_isr_cycles[n] = c;
    g_isr_n = n + 1;

    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(g_handler, &woken);
    // woken is true if the handler has a higher priority than the task this
    // interrupt stopped. Only then does a switch at ISR exit make sense.
    if (g_yield) portYIELD_FROM_ISR(woken);
}

static void ARDUINO_ISR_ATTR on_noisy(void) {
    if (g_noisy_defers) {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(g_worker, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        rt_busy_us(kNoisyWorkUs);  // "do the work in the ISR"
    }
}

// ----------------------------------------------------------- tasks --------

// Bookkeeping for one wake-up of the handler. `pending` is how many interrupts
// arrived since the last wake-up. Normally 1. If the handler was late, several
// interrupts share one wake-up; the oldest of them has waited longest, so
// that is the one whose latency is stored.
static void record_wakeup(uint32_t pending, uint32_t woke_cycles) {
    int oldest = g_consumed;
    g_consumed += pending;
    if (pending > 1) g_merged += pending - 1;
    int n = g_task_n;
    if (n < kSamples && oldest < kSamples) {
        g_task_cycles[n] = woke_cycles - g_isr_cycles[oldest];
        g_task_n = n + 1;
    }
}

// The handler: sleep until the ISR notifies us, note the time, "handle".
static void handler_task(void* arg) {
    (void)arg;  // not used
    for (;;) {
        uint32_t pending = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t c = rt_cycles();
        record_wakeup(pending, c);
    }
}

static void worker_task(void* arg) {  // variant f: the noisy ISR's deferred work
    (void)arg;  // not used
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        rt_busy_us(kNoisyWorkUs);
    }
}

static void busy_task(void* arg) {  // variant c: computes 3 ms, sleeps 2 ms
    (void)arg;  // not used
    while (!g_stop) {
        rt_busy_us(3000);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void critical_task(void* arg) {  // variant d: interrupts off 200 us at a time
    (void)arg;  // not used
    static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    while (!g_stop) {
        portENTER_CRITICAL(&mux);
        rt_busy_us(kCriticalUs);
        portEXIT_CRITICAL(&mux);
        rt_busy_us(50);
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------- report --------

static void print_us(const char* what, const struct rt_stats* s, int32_t per_us) {
    Serial.printf("%-22s min %8.1f us, avg %8.1f us, max %8.1f us\n", what, s->min / (float)per_us,
                  rt_stats_avg(s) / (float)per_us, s->max / (float)per_us);
}

static void run(char key) {
    g_yield = (key != 'b');
    g_noisy_defers = (key == 'f');
    UBaseType_t handler_prio = (key == 'c') ? 2 : 10;

    Serial.printf("\n--- variant %c: handler priority %u, %s ---\n", key, (unsigned)handler_prio,
                  g_yield ? "ISR yields" : "ISR does NOT yield");

    // loop() must be able to stop the run even when a background task never
    // sleeps, so it runs above everything while it waits.
    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 15);

    g_isr_n = 0;
    g_task_n = 0;
    g_consumed = 0;
    g_merged = 0;
    g_stop = false;
    int helpers = 0;
    g_handler = rt_start_task(handler_task, "handler", handler_prio, NULL);
    if (key == 'c') {
        rt_start_task(busy_task, "busy", 5, NULL);
        helpers++;
    }
    if (key == 'd') {
        rt_start_task(critical_task, "critical", 3, NULL);
        helpers++;
    }
    if (key == 'e' || key == 'f') {
        if (key == 'f') g_worker = rt_start_task(worker_task, "worker", 5, NULL);
        g_noisy = timerBegin(1000000);
        if (g_noisy == NULL) rt_fatal("timerBegin failed");
        timerAttachInterrupt(g_noisy, on_noisy);
        timerAlarm(g_noisy, kNoisyPeriodUs, true, 0);
    }

    g_timer = timerBegin(kTimerHz);
    if (g_timer == NULL) rt_fatal("timerBegin failed");
    timerAttachInterrupt(g_timer, on_alarm);
    timerAlarm(g_timer, kAlarmCounts, true, 0);

    vTaskDelay(pdMS_TO_TICKS(kSamples + 50));  // the run

    timerEnd(g_timer);
    if (g_noisy) {
        timerEnd(g_noisy);
        g_noisy = NULL;
    }
    g_stop = true;
    for (int i = 0; i < helpers; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskDelete(g_handler);
    if (g_worker) {
        vTaskDelete(g_worker);
        g_worker = NULL;
    }
    vTaskPrioritySet(NULL, my_prio);

    struct rt_stats isr, task;
    rt_stats_init(&isr, 10);
    rt_stats_init(&task, 160);
    for (int i = 0; i < g_isr_n; i++) rt_stats_add(&isr, g_isr_counts[i]);
    for (int i = 0; i < g_task_n; i++) rt_stats_add(&task, (int32_t)g_task_cycles[i]);
    Serial.printf("%d interrupts, handler woke %d times (%lu events shared a wake-up)\n", g_isr_n, g_task_n,
                  (unsigned long)g_merged);
    print_us("interrupt latency", &isr, kTimerHz / 1000000);
    print_us("ISR -> handler task", &task, CPU_MHZ);

    char v[64];
    snprintf(v, sizeof(v), "ISR_MAX_US=%.1f TASK_MAX_US=%.1f", isr.max / (float)(kTimerHz / 1000000),
             task.max / (float)CPU_MHZ);
    rt_verdict(v);
}

static void menu(void) {
    Serial.println("\nkeys: a baseline | b no yield | c low-priority handler | d critical sections | "
                   "e long ISR | f deferred ISR work");
}

void setup() {
    rt_begin("lab 4: isr_latency");
    g_done = xSemaphoreCreateCounting(2, 0);

    // Reading the timer is not free, and its cost is in every number below.
    hw_timer_t* t = timerBegin(kTimerHz);
    timerRead(t);  // once to get it into the cache
    uint32_t c0 = rt_cycles();
    volatile uint64_t x = timerRead(t);
    uint32_t c = rt_cycles() - c0;
    (void)x;
    timerEnd(t);
    Serial.printf("one timerRead() costs %.2f us\n", c / (float)CPU_MHZ);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'f') {
        run((char)k);
        menu();
    }
    delay(20);
}
