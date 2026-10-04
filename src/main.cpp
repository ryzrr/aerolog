#include <Arduino.h>
#include "config.h"
#include "sensor.h"
#include "payload.h"
#include "net.h"

void setup() {
  Serial.begin(115200);
  if (!sensorBegin()) { Serial.println("[ERR] BME280 not found"); while (true) delay(1000); }  // FR6
  netBegin();
}

void loop() {
  netLoop();                                    // FR3: reconnect + mqtt.loop()
  static unsigned long last = 0;
  if (millis() - last >= PUBLISH_MS) {          // FR1
    last = millis();
    char msg[96];
    formatPayload(msg, sizeof msg, sensorRead());   // FR2
    netPublish(TOPIC_DATA, msg);
  }
}
