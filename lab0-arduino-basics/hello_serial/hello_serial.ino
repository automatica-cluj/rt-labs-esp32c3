// hello_serial - the structure of a sketch, and talking to it over USB
//
// setup() runs once after reset, loop() runs again and again. Serial is the
// USB link to the Serial Monitor: the board prints with Serial.print() and
// Serial.printf(), and reads the keys you type with Serial.read().
//
//   i  information about the board: chip, clock, memory, uptime, temperature
//   t  C types on this chip: their sizes, and what happens at their limits
//   f  the same numbers printed in different formats
//   any other key is echoed back with its character code
//
// This lab does not use rt.h: everything here is the plain Arduino API.

// ---- EXPERIMENT KNOBS ----
const uint32_t kBaud = 115200;  // must match the Serial Monitor (ignored over USB, but keep them equal)

static uint32_t g_loops = 0;  // how many times loop() has run

static void print_menu(void) {
    Serial.println("keys: i board info | t types | f formats | anything else is echoed");
}

static void show_info(void) {
    Serial.printf("chip            %s, revision %d\n", ESP.getChipModel(), (int)ESP.getChipRevision());
    Serial.printf("CPU clock       %lu MHz\n", (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("flash           %lu bytes\n", (unsigned long)ESP.getFlashChipSize());
    Serial.printf("free heap (RAM) %lu bytes\n", (unsigned long)ESP.getFreeHeap());
    Serial.printf("sketch size     %lu bytes\n", (unsigned long)ESP.getSketchSize());
    Serial.printf("uptime          %lu ms since reset\n", (unsigned long)millis());
    Serial.printf("loop() ran      %lu times so far\n", (unsigned long)g_loops);
    Serial.printf("chip temperature %.1f C (the die, not the room)\n", temperatureRead());

    char v[64];
    snprintf(v, sizeof(v), "CHIP=%s CPU_MHZ=%lu", ESP.getChipModel(), (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("VERDICT: %s\n", v);
}

static void show_types(void) {
    Serial.println("size of each type, in bytes:");
    Serial.printf("  char %d, short %d, int %d, long %d, long long %d\n", (int)sizeof(char), (int)sizeof(short),
                  (int)sizeof(int), (int)sizeof(long), (int)sizeof(long long));
    Serial.printf("  float %d, double %d, a pointer %d\n", (int)sizeof(float), (int)sizeof(double),
                  (int)sizeof(void*));

    // Fixed-size types say their width in the name. Unsigned ones wrap
    // around at their limit, with no error.
    uint8_t small = 255;
    small = small + 1;
    Serial.printf("uint8_t:  255 + 1 = %u\n", (unsigned)small);

    uint32_t zero = 0;
    uint32_t below = zero - 1;
    Serial.printf("uint32_t: 0 - 1   = %lu\n", (unsigned long)below);

    // Integer division drops the fraction; one float operand keeps it.
    int a = 7, b = 2;
    Serial.printf("int:      7 / 2   = %d,  7 %% 2 = %d\n", a / b, a % b);
    Serial.printf("float:    7 / 2.0 = %.2f\n", a / 2.0f);

    // float has about 7 significant digits.
    float big = 16777216.0f;  // 2^24
    Serial.printf("float:    16777216 + 1 = %.1f\n", big + 1.0f);

    char v[64];
    snprintf(v, sizeof(v), "SIZEOF_INT=%d SIZEOF_LONG=%d SIZEOF_PTR=%d", (int)sizeof(int), (int)sizeof(long),
             (int)sizeof(void*));
    Serial.printf("VERDICT: %s\n", v);
}

static void show_formats(void) {
    int n = 200;
    float x = 3.14159f;

    Serial.println("Serial.print with a base:");
    Serial.print("  decimal ");
    Serial.println(n);
    Serial.print("  hex     ");
    Serial.println(n, HEX);
    Serial.print("  binary  ");
    Serial.println(n, BIN);
    Serial.print("  float   ");
    Serial.println(x, 4);  // 4 digits after the point

    Serial.println("Serial.printf, the same format strings as printf in C:");
    Serial.printf("  %%d   [%d]\n", n);
    Serial.printf("  %%5d  [%5d]   width 5, right-aligned\n", n);
    Serial.printf("  %%-5d [%-5d]   width 5, left-aligned\n", n);
    Serial.printf("  %%x   [%x]    %%04X [%04X]\n", n, n);
    Serial.printf("  %%f   [%f]  %%.2f [%.2f]\n", x, x);
    Serial.printf("  %%c   [%c]      %%s [%s]\n", 'A', "text");

    Serial.println("VERDICT: FORMATS_DONE");
}

void setup() {
    Serial.begin(kBaud);
    // Over USB the board starts before the Serial Monitor is open. Wait for
    // it up to 3 s, so the first lines are not lost.
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);

    Serial.println();
    Serial.println("=== lab 0: hello_serial ===");
    print_menu();
}

void loop() {
    g_loops = g_loops + 1;

    if (Serial.available() > 0) {
        int k = Serial.read();
        if (k == 'i') {
            show_info();
        } else if (k == 't') {
            show_types();
        } else if (k == 'f') {
            show_formats();
        } else if (k != '\r' && k != '\n') {
            Serial.printf("you typed '%c', character code %d (0x%02X)\n", k, k, k);
        }
    }
    delay(10);
}
