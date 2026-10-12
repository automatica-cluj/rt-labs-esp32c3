// mqtt_hello - send and receive MQTT messages over Wi-Fi
//
// MQTT is a small publish/subscribe protocol. Devices do not talk to each
// other directly: each one connects to a broker, publishes messages on a
// topic (a name such as "rt-labs/a1b2c3/uptime"), and subscribes to the
// topics it wants to receive. The broker forwards every message to every
// subscriber of its topic.
//
// This sketch has no keys. Once connected, it
//   - publishes "uptime N s" on rt-labs/<id>/uptime once a second
//   - subscribes to rt-labs/<id>/led: "on" lights the LED, "off" turns it off
// and prints every message it sends or receives in the Serial Monitor.
// <id> is the last three bytes of the board's MAC address, printed at start.
//
// The MQTT client is ESP-IDF's (mqtt_client.h), part of the Arduino core, so
// no library has to be installed. It runs in its own FreeRTOS task and calls
// on_mqtt_event() for everything that happens: connected, message received...

#include <WiFi.h>

#include "mqtt_client.h"

// ---- EXPERIMENT KNOBS ----
const char* kWifiSsid = "iot-demo";  // the lab network; change it to use another one
const char* kWifiPassword = "demo123!";
const char* kBrokerUri = "mqtt://control.aut.utcluj.ro:11188";
const uint32_t kPublishMs = 1000;    // one uptime message per second
const int kLedPin = 8;               // blue LED, lit when LOW

static esp_mqtt_client_handle_t g_client = NULL;
static char g_id[8];             // the last 3 bytes of the MAC address, in hex
static char g_uptime_topic[40];  // rt-labs/<id>/uptime: we publish here
static char g_led_topic[40];     // rt-labs/<id>/led: we listen here
static volatile bool g_connected = false;

// true if the n bytes at s are exactly the text t
static bool same_text(const char* s, int n, const char* t) { return n == (int)strlen(t) && strncmp(s, t, n) == 0; }

// Called by the MQTT client's task, not by loop().
static void on_mqtt_event(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    (void)handler_arg;  // not used
    (void)base;
    esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)event_data;

    if (event_id == MQTT_EVENT_CONNECTED) {
        g_connected = true;
        Serial.printf("[mqtt] connected to %s\n", kBrokerUri);
        // Subscribe after every (re)connection: the broker forgets the
        // subscriptions of a client that went away.
        esp_mqtt_client_subscribe(g_client, g_led_topic, 0);
        Serial.printf("[mqtt] subscribed to %s\n", g_led_topic);
        Serial.println("VERDICT: MQTT=CONNECTED");
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        g_connected = false;
        Serial.println("[mqtt] disconnected, the client retries by itself");
    } else if (event_id == MQTT_EVENT_DATA) {
        // topic and data are not 0-terminated: print them with their lengths.
        Serial.printf("[mqtt] received on %.*s: %.*s\n", ev->topic_len, ev->topic, ev->data_len, ev->data);
        if (same_text(ev->data, ev->data_len, "on")) {
            digitalWrite(kLedPin, LOW);
            Serial.println("LED on");
        } else if (same_text(ev->data, ev->data_len, "off")) {
            digitalWrite(kLedPin, HIGH);
            Serial.println("LED off");
        } else {
            Serial.println("(not \"on\" or \"off\", ignored)");
        }
    } else if (event_id == MQTT_EVENT_ERROR) {
        Serial.println("[mqtt] error (broker unreachable? wrong host or port?)");
    }
}

static void connect_wifi(void) {
    Serial.printf("connecting to Wi-Fi \"%s\"", kWifiSsid);
    WiFi.begin(kWifiSsid, kWifiPassword);
    // The Super Mini's small antenna often connects better at lower power.
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    uint32_t t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Wi-Fi connected, IP address %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("Wi-Fi did not connect: check kWifiSsid and kWifiPassword, then upload again");
    }
}

static void start_mqtt(void) {
    static char client_id[24];
    snprintf(client_id, sizeof(client_id), "rt-labs-%s", g_id);

    esp_mqtt_client_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.broker.address.uri = kBrokerUri;
    cfg.credentials.client_id = client_id;

    g_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(g_client, MQTT_EVENT_ANY, on_mqtt_event, NULL);
    esp_mqtt_client_start(g_client);  // connects in the background
}

void setup() {
    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, HIGH);  // off

    Serial.begin(115200);
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    delay(200);
    Serial.println();
    Serial.println("=== lab 0: mqtt_hello ===");

    WiFi.mode(WIFI_STA);  // station: a client of an access point
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(g_id, sizeof(g_id), "%02x%02x%02x", mac[3], mac[4], mac[5]);
    snprintf(g_uptime_topic, sizeof(g_uptime_topic), "rt-labs/%s/uptime", g_id);
    snprintf(g_led_topic, sizeof(g_led_topic), "rt-labs/%s/led", g_id);
    Serial.printf("board id %s: publishing on %s, listening on %s\n", g_id, g_uptime_topic, g_led_topic);

    connect_wifi();
    if (WiFi.status() == WL_CONNECTED) start_mqtt();
}

void loop() {
    if (g_connected) {
        char msg[32];
        snprintf(msg, sizeof(msg), "uptime %lu s", (unsigned long)(millis() / 1000));
        // len 0: the client takes strlen(msg). QoS 0, retain 0.
        esp_mqtt_client_publish(g_client, g_uptime_topic, msg, 0, 0, 0);
        Serial.printf("[mqtt] published on %s: %s\n", g_uptime_topic, msg);
    }
    delay(kPublishMs);
}
