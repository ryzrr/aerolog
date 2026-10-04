#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "config.h"
#include "sensor.h"

static Adafruit_BME280 bme;

bool sensorBegin() {
  Wire.begin(I2C_SDA, I2C_SCL);
  if (bme.begin(BME_ADDR_PRIMARY, &Wire)) return true;
  Serial.println("[WARN] BME280 not at 0x76, trying 0x77");
  return bme.begin(BME_ADDR_SECONDARY, &Wire);
}

Reading sensorRead() {
  return { bme.readTemperature(), bme.readHumidity(), bme.readPressure() / 100.0F };
}
