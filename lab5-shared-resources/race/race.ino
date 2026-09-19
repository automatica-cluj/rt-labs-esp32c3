// race - two tasks, one counter, lost updates
//
// Two tasks each add 1 to the same counter one million times. The result
// should be 2 000 000. `counter = counter + 1` is three instructions: load,
// add, store. If the scheduler switches tasks between the load and the store,
// the other task's work in the meantime is overwritten: a lost update.
//
//   a  no protection, both tasks at priority 3 (the tick rotates them)
//   b  no protection, priorities 3 and 4
//   c  a FreeRTOS mutex around every increment, priority 3 and 3
//   d  a critical section (interrupts off) around every increment
//   e  std::atomic<uint32_t> and fetch_add
//
// Every variant prints the final count, how many updates were lost and how
// long the whole thing took.

#include <atomic>

#include "rtlab.h"

// ---- EXPERIMENT KNOBS ----
const uint32_t kIncrements = 1000000;  // per task

static volatile uint32_t g_counter;   // variants a-d
static std::atomic<uint32_t> g_atomic;  // variant e
static SemaphoreHandle_t g_mutex;
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;  // needed by portENTER_CRITICAL (interrupts off)
static SemaphoreHandle_t g_done;
static char g_mode;

static void adder_task(void*) {
    for (uint32_t i = 0; i < kIncrements; i++) {
        switch (g_mode) {
            case 'c':
                xSemaphoreTake(g_mutex, portMAX_DELAY);
                g_counter = g_counter + 1;
                xSemaphoreGive(g_mutex);
                break;
            case 'd':
                portENTER_CRITICAL(&g_mux);
                g_counter = g_counter + 1;
                portEXIT_CRITICAL(&g_mux);
                break;
            case 'e':
                g_atomic.fetch_add(1);
                break;
            default:  // a, b: the race
                g_counter = g_counter + 1;
        }
    }
    xSemaphoreGive(g_done);
    vTaskDelete(NULL);
}

static void run(char key) {
    g_mode = key;
    g_counter = 0;
    g_atomic.store(0);
    UBaseType_t p1 = 3, p2 = (key == 'b') ? 4 : 3;

    Serial.printf("\n--- variant %c: priorities %u and %u ---\n", key, (unsigned)p1, (unsigned)p2);
    UBaseType_t my_prio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, 10);  // create both before either starts
    int64_t t0 = now_us();
    start_task(adder_task, "adder1", p1);
    start_task(adder_task, "adder2", p2);
    vTaskPrioritySet(NULL, my_prio);  // now they run
    xSemaphoreTake(g_done, portMAX_DELAY);
    xSemaphoreTake(g_done, portMAX_DELAY);
    int64_t took = now_us() - t0;

    uint32_t result = (key == 'e') ? g_atomic.load() : g_counter;
    uint32_t expected = 2 * kIncrements;
    Serial.printf("expected %lu, got %lu, lost %lu updates (%.2f %%), took %.1f ms\n", (unsigned long)expected,
                  (unsigned long)result, (unsigned long)(expected - result), 100.0 * (expected - result) / expected,
                  took / 1000.0);
    char v[32];
    snprintf(v, sizeof(v), "LOST=%lu", (unsigned long)(expected - result));
    verdict(v);
}

static void menu(void) {
    Serial.println("\nkeys: a race | b race, different priorities | c mutex | d critical section | e atomic");
}

void setup() {
    rtlab_begin("lab 5: race");
    g_mutex = xSemaphoreCreateMutex();
    g_done = xSemaphoreCreateCounting(2, 0);
    menu();
}

void loop() {
    int k = read_key();
    if (k >= 'a' && k <= 'e') {
        run((char)k);
        menu();
    }
    delay(20);
}
