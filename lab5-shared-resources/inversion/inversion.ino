// inversion - priority inversion, and priority inheritance fixing it
//
// Three tasks share one CPU. L and H share a lock; M does not use it.
//
//    0 ms  L (priority 2) takes the lock and computes 40 ms inside it
//   10 ms  H (priority 5) wants the lock and has to wait for L
//   20 ms  M (priority 3) computes 150 ms, no lock
//
// If the lock is a binary semaphore, M preempts L, so H waits for M as well:
// H is blocked by a task with a lower priority that has nothing to do with the
// lock. With a FreeRTOS mutex, L inherits H's priority while H waits, so M
// cannot get in between, and H waits at most for the rest of L's critical
// section.
//
//   a  binary semaphore (no priority inheritance)
//   b  mutex (priority inheritance)
//   c  binary semaphore, and a second medium task N (priority 4) at 25 ms
//   d  mutex, with N as well
//
// "blocked" is the time from H asking for the lock until H getting it.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kCsMs = 40;       // L's critical section
const uint32_t kMediumMs = 150;  // each medium task's computation
const uint32_t kHighMs = 5;      // H's work inside the lock
const UBaseType_t kPrioL = 2, kPrioM = 3, kPrioN = 4, kPrioH = 5;

static SemaphoreHandle_t g_lock;
static SemaphoreHandle_t g_done;
static float g_h_request_ms, g_h_got_ms;
static UBaseType_t g_l_max_prio;  // highest priority L ran at inside the lock
static char g_letter_m = 'M', g_letter_n = 'N';  // timeline letters of the medium tasks

static void low_task(void* arg) {
    (void)arg;  // not used
    xSemaphoreTake(g_lock, portMAX_DELAY);
    for (uint32_t ms = 0; ms < kCsMs; ms++) {
        rt_tl_busy(1000, 'L');
        UBaseType_t p = uxTaskPriorityGet(NULL);
        if (p > g_l_max_prio) g_l_max_prio = p;
    }
    xSemaphoreGive(g_lock);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void medium_task(void* arg) {
    char letter = *(char*)arg;  // arg points to g_letter_m or g_letter_n
    rt_tl_busy(kMediumMs * 1000, letter);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void high_task(void* arg) {
    (void)arg;  // not used
    g_h_request_ms = rt_tl_ms();
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_h_got_ms = rt_tl_ms();
    rt_tl_busy(kHighMs * 1000, 'H');
    xSemaphoreGive(g_lock);
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void wait_until_ms(int ms) {
    int32_t left = ms - (int32_t)rt_tl_ms();
    if (left > 0) vTaskDelay(pdMS_TO_TICKS(left));
}

static void run(char key) {
    bool mutex = (key == 'b' || key == 'd');
    bool two = (key == 'c' || key == 'd');
    if (mutex) {
        g_lock = xSemaphoreCreateMutex();
    } else {
        g_lock = xSemaphoreCreateBinary();
        xSemaphoreGive(g_lock);  // a binary semaphore starts empty
    }
    g_l_max_prio = 0;

    Serial.printf("\n--- variant %c: %s, %s ---\n", key, mutex ? "mutex (priority inheritance)" : "binary semaphore",
                  two ? "two medium tasks" : "one medium task");

    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 10);  // releases happen on time
    rt_tl_start(5000, 80);
    rt_start_task(low_task, "L", kPrioL, NULL);
    wait_until_ms(10);
    rt_start_task(high_task, "H", kPrioH, NULL);
    wait_until_ms(20);
    rt_start_task(medium_task, "M", kPrioM, &g_letter_m);
    if (two) {
        wait_until_ms(25);
        rt_start_task(medium_task, "N", kPrioN, &g_letter_n);
    }
    for (int i = 0; i < (two ? 4 : 3); i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskPrioritySet(NULL, my_prio);
    vSemaphoreDelete(g_lock);

    rt_tl_print(80);
    float blocked = g_h_got_ms - g_h_request_ms;
    Serial.printf("H asked for the lock at %.1f ms and got it at %.1f ms: blocked %.1f ms\n", g_h_request_ms,
                  g_h_got_ms, blocked);
    Serial.printf("L's critical section is %lu ms; L ran at priority up to %u while holding the lock\n",
                  (unsigned long)kCsMs, (unsigned)g_l_max_prio);
    rt_verdict(blocked <= kCsMs + 1 ? "BOUNDED" : "UNBOUNDED");
}

static void menu(void) {
    Serial.println("\nkeys: a binary semaphore | b mutex | c semaphore, two mediums | d mutex, two mediums");
}

void setup() {
    rt_begin("lab 5: inversion");
    g_done = xSemaphoreCreateCounting(4, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'd') {
        run((char)k);
        menu();
    }
    delay(20);
}
