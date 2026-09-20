// rm_taskset - three periodic tasks under fixed priorities, theory vs board
//
// Each task is released every T ms and computes C ms (rt_busy_us). Its deadline
// is the next release. For each task set the sketch prints:
//   - the utilisation U and the Liu & Layland bound for n tasks
//   - the worst-case response time predicted by response-time analysis (RTA)
//   - what actually happened: response times, deadline misses, skipped jobs
//
// Keys pick the task set (edit the tables below to make your own):
//
//   a  U = 0.60, below the bound                  rate monotonic priorities
//   b  U = 0.85, above the bound                  rate monotonic priorities
//   c  U = 1.03, overload, late jobs are skipped  rate monotonic priorities
//   d  set b with the priorities reversed (the longest period gets the highest)
//   e  set c, but late jobs run late instead of being skipped (catch-up)
//
// All tasks are first released together at t0: the critical instant.

#include "rt.h"

// ---- EXPERIMENT KNOBS ----
struct Spec {
    const char* name;
    uint32_t T_ms;      // period, and relative deadline
    uint32_t C_ms;      // computation per job
    UBaseType_t prio;   // higher number = higher priority
};

const struct Spec kSetA[3] = {{"sensor", 10, 2, 6}, {"control", 20, 4, 5}, {"logger", 40, 8, 4}};
const struct Spec kSetB[3] = {{"sensor", 10, 3, 6}, {"control", 15, 4, 5}, {"logger", 35, 10, 4}};
const struct Spec kSetC[3] = {{"sensor", 10, 3, 6}, {"control", 15, 5, 5}, {"logger", 35, 14, 4}};
const struct Spec kSetD[3] = {{"sensor", 10, 3, 4}, {"control", 15, 4, 5}, {"logger", 35, 10, 6}};
const uint32_t kRunMs = 2100;         // 10 x the hyperperiod of sets b-e
const UBaseType_t kControlPrio = 10;  // loop() while it runs the experiment

struct Task {
    struct Spec s;
    // results
    uint32_t jobs, misses, skipped;
    int32_t r_max_us;
    int64_t r_sum_us;
    int32_t rta_us;  // -1 = RTA says the deadline can be missed
};

static struct Task g_task[3];
static bool g_catch_up;
static volatile bool g_stop;
static TickType_t g_t0_tick;
static int64_t g_t0_us;
static SemaphoreHandle_t g_done;

// Job k of a task is released at t0 + k * T. The same moment is needed twice:
// in microseconds, to measure the response time, and in ticks, to sleep.
static int64_t release_us(const struct Task* t, int64_t k) { return g_t0_us + k * (int64_t)t->s.T_ms * 1000; }

// Sleep until job k is released. If that moment has already passed, this
// returns at once, which is what makes the overrun policies below work.
static void sleep_until_release(const struct Task* t, int64_t k) {
    TickType_t T_ticks = pdMS_TO_TICKS(t->s.T_ms);
    rt_sleep_until_tick(g_t0_tick + (TickType_t)k * T_ticks);
}

// This is the whole periodic task: wait for the release, compute, note the
// response time, pick the next job.
static void periodic_task(void* arg) {
    struct Task* t = (struct Task*)arg;
    int64_t T_us = (int64_t)t->s.T_ms * 1000;
    int64_t k = 0;  // number of the current job

    sleep_until_release(t, k);
    while (!g_stop) {
        rt_busy_us(t->s.C_ms * 1000);                          // the job
        int32_t r = (int32_t)(rt_now_us() - release_us(t, k));  // its response time

        t->jobs++;
        t->r_sum_us += r;
        if (r > t->r_max_us) t->r_max_us = r;
        if (r > T_us) t->misses++;  // deadline = next release
        k++;

        // Overrun policy "skip": jobs whose release has already passed are
        // dropped; go straight to the next release in the future.
        // With "catch-up" they stay, and the sleep below returns at once.
        if (!g_catch_up) {
            while (release_us(t, k) <= rt_now_us()) {
                k++;
                t->skipped++;
            }
        }
        sleep_until_release(t, k);
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

// Response-time analysis: R = C_i + sum over higher-priority j of
// ceil(R / T_j) * C_j, iterated until it stops changing.
static int32_t rta(int i, int n) {
    int32_t C = g_task[i].s.C_ms * 1000, D = g_task[i].s.T_ms * 1000;
    int32_t R = C, prev = 0;
    while (R != prev && R <= D) {
        prev = R;
        R = C;
        for (int j = 0; j < n; j++) {
            if (g_task[j].s.prio <= g_task[i].s.prio) continue;
            int32_t Tj = g_task[j].s.T_ms * 1000;
            R += ((prev + Tj - 1) / Tj) * (int32_t)(g_task[j].s.C_ms * 1000);
        }
    }
    return R <= D ? R : -1;
}

// Everything below only prints. One table row per task, then the verdict:
// SCHEDULABLE, or the number of misses of each task.
static void print_results(int n) {
    Serial.println("task     T ms  C ms  prio     U | RTA R ms | meas. R max  R avg |  jobs  miss  skip");
    uint32_t misses = 0;
    char v[96] = "MISSES";
    for (int i = 0; i < n; i++) {
        struct Task* t = &g_task[i];
        char rta_txt[12];
        if (t->rta_us < 0) snprintf(rta_txt, sizeof(rta_txt), "  > D");
        else snprintf(rta_txt, sizeof(rta_txt), "%6.2f", t->rta_us / 1000.0);
        float r_avg_ms = t->jobs ? (float)(t->r_sum_us / t->jobs) / 1000.0f : 0.0f;

        Serial.printf("%-8s %4lu  %4lu  %4u  %.3f |", t->s.name, (unsigned long)t->s.T_ms,
                      (unsigned long)t->s.C_ms, (unsigned)t->s.prio, t->s.C_ms / (float)t->s.T_ms);
        Serial.printf("   %s |     %6.2f %6.2f |", rta_txt, t->r_max_us / 1000.0, r_avg_ms);
        Serial.printf(" %5lu %5lu %5lu\n", (unsigned long)t->jobs, (unsigned long)t->misses,
                      (unsigned long)t->skipped);

        misses += t->misses;
        // add " name=misses" to the end of the verdict text
        size_t len = strlen(v);
        snprintf(v + len, sizeof(v) - len, " %s=%lu", t->s.name, (unsigned long)t->misses);
    }
    rt_verdict(misses == 0 ? "SCHEDULABLE" : v);
}

static void run(char key) {
    const struct Spec* set = kSetA;
    if (key == 'b') set = kSetB;
    if (key == 'c' || key == 'e') set = kSetC;
    if (key == 'd') set = kSetD;
    g_catch_up = (key == 'e');
    const int n = 3;

    float U = 0;
    for (int i = 0; i < n; i++) {
        memset(&g_task[i], 0, sizeof(struct Task));
        g_task[i].s = set[i];
        U += set[i].C_ms / (float)set[i].T_ms;
    }
    float bound = n * (powf(2.0f, 1.0f / n) - 1.0f);
    for (int i = 0; i < n; i++) g_task[i].rta_us = rta(i, n);

    Serial.printf("\n--- task set %c, overrun policy: %s ---\n", key, g_catch_up ? "catch-up" : "skip");
    Serial.printf("U = %.3f, Liu & Layland bound for %d tasks = %.3f -> %s\n", U, n, bound,
                  U <= bound ? "schedulable (sufficient test)" : "the bound test says nothing");

    // Release all tasks together at t0, 20 ms from now.
    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, kControlPrio);
    g_stop = false;
    g_t0_tick = xTaskGetTickCount() + pdMS_TO_TICKS(20);
    for (int i = 0; i < n; i++) rt_start_task(periodic_task, g_task[i].s.name, g_task[i].s.prio, &g_task[i]);
    rt_sleep_until_tick(g_t0_tick);  // wake at t0, before the tasks (higher priority)
    g_t0_us = rt_now_us();
    vTaskDelay(pdMS_TO_TICKS(kRunMs));
    g_stop = true;
    for (int i = 0; i < n; i++) xSemaphoreTake(g_done, portMAX_DELAY);
    vTaskPrioritySet(NULL, my_prio);

    print_results(n);
}

static void menu(void) {
    Serial.println("\nkeys: a U=0.60 | b U=0.85 | c overload, skip | d b with wrong priorities | e overload, catch-up");
}

void setup() {
    rt_begin("lab 3: rm_taskset");
    g_done = xSemaphoreCreateCounting(3, 0);
    menu();
}

void loop() {
    int k = rt_read_key();
    if (k >= 'a' && k <= 'e') {
        run((char)k);
        menu();
    }
    delay(20);
}
