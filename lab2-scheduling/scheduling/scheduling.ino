// scheduling - watch the FreeRTOS scheduler decide who gets the one CPU
//
// Worker tasks burn CPU time and mark a timeline while they run. The control
// code (loop(), priority 10 while it releases workers) creates them at known
// moments, then waits and prints the timeline. Keys pick the scenario:
//
//   a  preemption: L (prio 2) at 0 ms, M (prio 3) at 40 ms, H (prio 4) at 80 ms,
//      each computes 100 ms
//   b  equal priorities: A, B, C all at priority 3, released together,
//      40 ms each; the tick (1 ms) rotates them
//   c  a periodic task: H (prio 4) computes 4 ms every 20 ms, while
//      L (prio 2) computes 120 ms in one go
//   d  like a, and a snapshot of every task in the system taken at 60 ms
//
// In the timeline strip each column is a slice of time and the letter is the
// task that was running in it.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const UBaseType_t kControlPrio = 10;  // above every worker

struct Worker {
    char letter;
    UBaseType_t prio;
    uint32_t work_us;   // total computation
    int jobs;           // 1 = one-shot; more = periodic
    int period_ms;      // for periodic workers
    float release_ms, start_ms, finish_ms;  // measured, from rt_tl_start()
};

static struct Worker g_w[3];
static int g_nw;
static SemaphoreHandle_t g_done;
static char g_snapshot[1200];

static void worker_task(void* arg) {
    struct Worker* w = (struct Worker*)arg;
    w->start_ms = rt_tl_ms();
    TickType_t last = xTaskGetTickCount();
    for (int j = 0; j < w->jobs; j++) {
        if (j > 0) xTaskDelayUntil(&last, pdMS_TO_TICKS(w->period_ms));
        rt_tl_busy(w->work_us, w->letter);
    }
    w->finish_ms = rt_tl_ms();
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void add_worker(char letter, UBaseType_t prio, uint32_t work_ms, int jobs, int period_ms) {
    struct Worker* w = &g_w[g_nw++];
    w->letter = letter;
    w->prio = prio;
    w->work_us = work_ms * 1000;
    w->jobs = jobs;
    w->period_ms = period_ms;
}

static void release(struct Worker* w) {
    char name[2] = {w->letter, 0};
    w->release_ms = rt_tl_ms();
    rt_start_task(worker_task, name, w->prio, w);
}

// Sleep until `ms` after rt_tl_start(). The control code has the highest
// priority, so it wakes on time and releases the next worker on time.
static void wait_until_ms(int ms) {
    int32_t left = ms - (int32_t)rt_tl_ms();
    if (left > 0) vTaskDelay(pdMS_TO_TICKS(left));
}

// The verdict: the workers' letters, earliest finish first, e.g. "HML".
// Only reporting: pick the earliest finisher not yet listed, g_nw times.
static void finish_order(char* order) {
    bool listed[3] = {false, false, false};
    for (int n = 0; n < g_nw; n++) {
        int best = -1;
        for (int i = 0; i < g_nw; i++) {
            if (listed[i]) continue;
            if (best < 0 || g_w[i].finish_ms < g_w[best].finish_ms) best = i;
        }
        listed[best] = true;
        order[n] = g_w[best].letter;
    }
    order[g_nw] = 0;  // end of string
}

static void run(char key) {
    g_nw = 0;
    int slice_us = 5000, columns = 70;
    int release_at[3] = {0, 0, 0};

    if (key == 'a' || key == 'd') {
        add_worker('L', 2, 100, 1, 0);
        add_worker('M', 3, 100, 1, 0);
        add_worker('H', 4, 100, 1, 0);
        release_at[1] = 40;
        release_at[2] = 80;
    } else if (key == 'b') {
        add_worker('A', 3, 40, 1, 0);
        add_worker('B', 3, 40, 1, 0);
        add_worker('C', 3, 40, 1, 0);
        slice_us = 1000;
        columns = 130;
    } else {  // 'c'
        add_worker('L', 2, 120, 1, 0);
        add_worker('H', 4, 4, 8, 20);
        slice_us = 1000;
        columns = 170;
    }

    Serial.printf("\n--- scenario %c ---\n", key);
    for (int i = 0; i < g_nw; i++)
        Serial.printf("%c: priority %u, released at %3d ms, %d job(s) of %lu ms%s\n", g_w[i].letter,
                      (unsigned)g_w[i].prio, release_at[i], g_w[i].jobs, (unsigned long)(g_w[i].work_us / 1000),
                      g_w[i].jobs > 1 ? ", periodic" : "");

    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, kControlPrio);
    rt_tl_start(slice_us, columns);
    for (int i = 0; i < g_nw; i++) {
        wait_until_ms(release_at[i]);
        release(&g_w[i]);
    }
    if (key == 'd') {
        wait_until_ms(60);
        vTaskList(g_snapshot);  // name, state, priority, least free stack (bytes), number
    }
    for (int i = 0; i < g_nw; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskPrioritySet(NULL, my_prio);

    rt_tl_print(100);
    Serial.println("task  prio  release   start  finish  response (ms)");
    for (int i = 0; i < g_nw; i++) {
        struct Worker* w = &g_w[i];
        Serial.printf("  %c   %4u  %7.1f %7.1f %7.1f  %8.1f\n", w->letter, (unsigned)w->prio, w->release_ms,
                      w->start_ms, w->finish_ms, w->finish_ms - w->release_ms);
    }
    if (key == 'd') {
        Serial.println("\nsnapshot at 60 ms (X running, R ready, B blocked, S suspended, D deleted)");
        Serial.println("name          state prio stack  #");
        Serial.print(g_snapshot);
    }

    char order[4];
    finish_order(order);
    char v[32];
    snprintf(v, sizeof(v), "FINISH_ORDER=%s", order);
    rt_verdict(v);
}

static void menu(void) {
    Serial.println("\nkeys: a preemption | b equal priorities | c periodic + long task | d a + task snapshot");
}

void setup() {
    rt_begin("lab 2: scheduling");
    g_done = xSemaphoreCreateCounting(3, 0);
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
