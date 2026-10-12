// mqtt_hello - send and receive MQTT messages over Wi-Fi
//
// MQTT is a small publish/subscribe protocol. Clients do not talk to each
// other directly: each one connects to a broker, publishes messages on a
// topic (a name such as "rt-labs/a1b2c3"), and subscribes to the topics it
// wants to receive. The broker forwards every message to every subscriber of
// its topic.
//
// This board subscribes to its own topic and to a topic shared by the whole
// class, so a message it publishes on its own topic comes straight back.
// Every message received is printed in the Serial Monitor.
//
//   p  publish "hello N" on this board's topic; it comes back through the broker
//   a  publish "hello N" on the class topic: every board in the room gets it
//   s  status: Wi-Fi, IP address, broker connection, topics
//
// kWifiSsid and kWifiPassword below are set for the lab network.
//
// The MQTT client is ESP-IDF's (mqtt_client.h), part of the Arduino core, so
// no library has to be installed. It runs in its own FreeRTOS task and calls
// on_mqtt_event() for everything that happens: connected, data received...

#include <WiFi.h>

#include "mqtt_client.h"

// ---- EXPERIMENT KNOBS ----
const char* kWifiSsid = "iot-demo";  // the lab network; change it to use another one
const char* kWifiPassword = "demo123!";
const char* kBrokerUri = "mqtt://control.aut.utcluj.ro:11188";
const char* kClassTopic = "rt-labs/all";  // every board subscribes to this one

static esp_mqtt_client_handle_t g_client = NULL;
static char g_id[8];          // the last 3 bytes of the MAC address, in hex
static char g_topic[32];      // this board's own topic: rt-labs/<id>
static volatile bool g_connected = false;
static uint32_t g_count = 0;  // number in the next "hello N"

// For key p: when the message left, so the echo can tell the round trip time
static volatile bool g_waiting_echo = false;
static volatile uint32_t g_sent_ms = 0;

// Called by the MQTT client's task, not by loop().
static void on_mqtt_event(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    (void)handler_arg;  // not used
    (void)base;
    esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)event_data;

    if (event_id == MQTT_EVENT_CONNECTED) {
        g_connected = true;
        Serial.printf("[mqtt] connected to %s\n", kBrokerUri);
        // Subscribing is done after every (re)connection: the broker forgets
        // the subscriptions of a client that went away.
        esp_mqtt_client_subscribe(g_client, g_topic, 0);
        esp_mqtt_client_subscribe(g_client, kClassTopic, 0);
        Serial.printf("[mqtt] subscribed to %s and %s\n", g_topic, kClassTopic);
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        g_connected = false;
        Serial.println("[mqtt] disconnected, the client retries by itself");
    } else if (event_id == MQTT_EVENT_DATA) {
        uint32_t now = millis();
        // topic and data are not 0-terminated: print them with their lengths.
        Serial.printf("[mqtt] received on %.*s: %.*s\n", ev->topic_len, ev->topic, ev->data_len, ev->data);

        bool own_topic = ev->topic_len == (int)strlen(g_topic) && strncmp(ev->topic, g_topic, ev->topic_len) == 0;
        if (own_topic && g_waiting_echo) {
            g_waiting_echo = false;
            Serial.printf("[mqtt] there and back through the broker in %lu ms\n", (unsigned long)(now - g_sent_ms));
            Serial.printf("VERDICT: MQTT_ECHO_MS=%lu\n", (unsigned long)(now - g_sent_ms));
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

static void publish(const char* topic) {
    // Key p right after reset may come before the broker connection is up.
    uint32_t t = millis();
    while (!g_connected && millis() - t < 10000) delay(100);
    if (!g_connected) {
        Serial.println("not connected to the broker");
        Serial.println("VERDICT: NOT_CONNECTED");
        return;
    }

    char msg[48];
    g_count = g_count + 1;
    snprintf(msg, sizeof(msg), "hello %lu from %s", (unsigned long)g_count, g_id);

    if (strcmp(topic, g_topic) == 0) {
        g_sent_ms = millis();
        g_waiting_echo = true;
    }
    // Print first: the echo can arrive before esp_mqtt_client_publish() returns.
    Serial.printf("[mqtt] publishing on %s: %s\n", topic, msg);
    // len 0: the client takes strlen(msg). QoS 0, retain 0.
    esp_mqtt_client_publish(g_client, topic, msg, 0, 0, 0);
    if (strcmp(topic, kClassTopic) == 0) Serial.println("VERDICT: PUBLISHED_ALL");
}

static void show_status(void) {
    Serial.printf("board id %s, own topic %s, class topic %s\n", g_id, g_topic, kClassTopic);
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Wi-Fi connected to \"%s\", IP %s, signal %d dBm\n", kWifiSsid,
                      WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
    } else {
        Serial.println("Wi-Fi not connected");
    }
    Serial.printf("broker %s: %s\n", kBrokerUri, g_connected ? "connected" : "not connected");
    Serial.printf("VERDICT: MQTT=%s\n", g_connected ? "CONNECTED" : "NOT_CONNECTED");
}

void setup() {
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
    snprintf(g_topic, sizeof(g_topic), "rt-labs/%s", g_id);

    connect_wifi();
    if (WiFi.status() == WL_CONNECTED) start_mqtt();
    Serial.println("keys: p publish on own topic | a publish to the class | s status");
}

void loop() {
    if (Serial.available() > 0) {
        int k = Serial.read();
        if (k == 'p') {
            publish(g_topic);
        } else if (k == 'a') {
            publish(kClassTopic);
        } else if (k == 's') {
            show_status();
        }
    }
    delay(10);
}
