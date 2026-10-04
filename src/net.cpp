#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "secrets.h"
#include "net.h"

static WiFiClient net;
static PubSubClient mqtt(net);
static unsigned long lastRetry = 0;

void netBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("[INFO] Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.print("[INFO] WiFi up, IP ");
  Serial.println(WiFi.localIP());
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
}

void netLoop() {
  if (WiFi.status() != WL_CONNECTED) {            // FR3: WiFi dropped
    if (millis() - lastRetry >= RETRY_MS) {
      lastRetry = millis();
      Serial.println("[WARN] WiFi lost, reconnecting");
      WiFi.reconnect();
    }
    return;
  }
  if (!mqtt.connected()) {                        // FR3: broker dropped
    if (millis() - lastRetry < RETRY_MS) return;
    lastRetry = millis();
    // FR4: Last Will "offline" (QoS 1, retained)
    if (!mqtt.connect(MQTT_CLIENT_ID, nullptr, nullptr, TOPIC_STATUS, 1, true, "offline")) {
      Serial.printf("[WARN] MQTT connect failed, rc=%d\n", mqtt.state());
      return;
    }
    Serial.println("[INFO] MQTT connected");
    mqtt.publish(TOPIC_STATUS, "online", true);   // FR4
  }
  mqtt.loop();
}

bool netPublish(const char* topic, const char* msg) {
  if (!mqtt.connected()) {
    Serial.println("[WARN] publish skipped, MQTT offline");
    return false;
  }
  if (!mqtt.publish(topic, msg)) {
    Serial.println("[ERR] publish failed");
    return false;
  }
  Serial.println(msg);
  return true;
}
