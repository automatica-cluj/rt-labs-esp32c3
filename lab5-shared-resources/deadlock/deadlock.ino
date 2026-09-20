// deadlock - two tasks, two mutexes, opposite order
//
//   task A: take m1, work, take m2, work, give both
//   task B: take m2, work, take m1, work, give both      <- opposite order
//
//   a  wait forever (portMAX_DELAY) and a 20 ms pause between the two takes:
//      deadlock. After 1 s the sketch shows who holds what and who waits.
//   b  the same, but every take gives up after 100 ms. A task that times out
//      releases what it holds, waits a random moment and starts again.
//   c  global lock order: B also takes m1 first. No deadlock possible.
//   d  no pause between the takes, 200 rounds each, 50 ms timeout: count how
//      often the deadlock happens when it depends on timing alone.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
const UBaseType_t kPrio = 3;  // A and B have the same priority

struct Worker {
    char name;
    SemaphoreHandle_t first;   // the mutex this task takes first
    SemaphoreHandle_t second;  // ... and second
    TickType_t timeout;   // portMAX_DELAY = wait forever
    uint32_t pause_ms;    // between taking the first and the second mutex
    int rounds;
    // results
    int done_rounds;
    int timeouts;
    TaskHandle_t handle;
};

static SemaphoreHandle_t g_m1, g_m2;
static SemaphoreHandle_t g_done;
static struct Worker g_a, g_b;

static void worker_task(void* arg) {
    struct Worker* w = (struct Worker*)arg;
    while (w->done_rounds < w->rounds) {
        if (xSemaphoreTake(w->first, w->timeout) != pdTRUE) {
            w->timeouts++;
            continue;
        }
        if (w->pause_ms) vTaskDelay(pdMS_TO_TICKS(w->pause_ms));
        else rt_busy_us(random(0, 500));
        if (xSemaphoreTake(w->second, w->timeout) != pdTRUE) {
            // Detected: we hold one and cannot get the other. Back off.
            w->timeouts++;
            xSemaphoreGive(w->first);
            vTaskDelay(pdMS_TO_TICKS(random(1, 30)));
            continue;
        }
        rt_busy_us(1000);  // work with both resources
        xSemaphoreGive(w->second);
        xSemaphoreGive(w->first);
        w->done_rounds++;
    }
    xSemaphoreGive(g_done);
    vTaskSuspend(NULL);  // loop() deletes us
}

static void setup_worker(struct Worker* w, char name, SemaphoreHandle_t first, SemaphoreHandle_t second,
                         TickType_t timeout, uint32_t pause_ms, int rounds) {
    w->name = name;
    w->first = first;
    w->second = second;
    w->timeout = timeout;
    w->pause_ms = pause_ms;
    w->rounds = rounds;
    w->done_rounds = 0;
    w->timeouts = 0;
}

static const char* holder_name(SemaphoreHandle_t m) {
    TaskHandle_t h = xSemaphoreGetMutexHolder(m);
    return h ? pcTaskGetName(h) : "nobody";
}

static const char* state_name(TaskHandle_t t) {
    switch (eTaskGetState(t)) {
        case eRunning: return "running";
        case eReady: return "ready";
        case eBlocked: return "blocked";
        case eSuspended: return "suspended (finished)";
        default: return "deleted";
    }
}

static void run(char key) {
    g_m1 = xSemaphoreCreateMutex();
    g_m2 = xSemaphoreCreateMutex();
    TickType_t forever = portMAX_DELAY;
    if (key == 'a') {
        setup_worker(&g_a, 'A', g_m1, g_m2, forever, 20, 1);
        setup_worker(&g_b, 'B', g_m2, g_m1, forever, 20, 1);
    } else if (key == 'b') {
        setup_worker(&g_a, 'A', g_m1, g_m2, pdMS_TO_TICKS(100), 20, 1);
        setup_worker(&g_b, 'B', g_m2, g_m1, pdMS_TO_TICKS(100), 20, 1);
    } else if (key == 'c') {
        setup_worker(&g_a, 'A', g_m1, g_m2, forever, 20, 1);
        setup_worker(&g_b, 'B', g_m1, g_m2, forever, 20, 1);
    } else {
        setup_worker(&g_a, 'A', g_m1, g_m2, pdMS_TO_TICKS(50), 0, 200);
        setup_worker(&g_b, 'B', g_m2, g_m1, pdMS_TO_TICKS(50), 0, 200);
    }
    Serial.printf("\n--- variant %c ---\n", key);

    int64_t t0 = rt_now_us();
    g_a.handle = rt_start_task(worker_task, "A", kPrio, &g_a);
    g_b.handle = rt_start_task(worker_task, "B", kPrio, &g_b);

    // Wait for both, but not forever: that is the deadlock detector.
    int finished = 0;
    while (finished < 2 && xSemaphoreTake(g_done, pdMS_TO_TICKS(key == 'd' ? 30000 : 1000)) == pdTRUE) finished++;
    float took = (rt_now_us() - t0) / 1000.0f;

    if (finished < 2) {
        Serial.println("after 1 s not both tasks have finished. Who holds what:");
        Serial.printf("  m1 is held by %s, m2 is held by %s\n", holder_name(g_m1), holder_name(g_m2));
        Serial.printf("  A is %s, B is %s\n", state_name(g_a.handle), state_name(g_b.handle));
        Serial.println("  A waits for m2 (held by B), B waits for m1 (held by A): a cycle.");
    } else {
        Serial.printf("both finished after %.1f ms\n", took);
    }
    Serial.printf("A: %d/%d rounds, %d timeouts | B: %d/%d rounds, %d timeouts\n", g_a.done_rounds, g_a.rounds,
                  g_a.timeouts, g_b.done_rounds, g_b.rounds, g_b.timeouts);

    char v[48];
    if (finished < 2) snprintf(v, sizeof(v), "DEADLOCK");
    else if (g_a.timeouts + g_b.timeouts > 0) snprintf(v, sizeof(v), "RECOVERED timeouts=%d", g_a.timeouts + g_b.timeouts);
    else snprintf(v, sizeof(v), "NO_DEADLOCK");
    rt_verdict(v);

    // Clean up. A deadlocked task can only be removed from outside.
    vTaskDelete(g_a.handle);
    vTaskDelete(g_b.handle);
    vSemaphoreDelete(g_m1);
    vSemaphoreDelete(g_m2);
}

static void menu(void) {
    Serial.println("\nkeys: a deadlock | b timeout and back off | c lock order | d timing-dependent, 200 rounds");
}

void setup() {
    rt_begin("lab 5: deadlock");
    g_done = xSemaphoreCreateCounting(2, 0);
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
