#pragma once

#define BME_ADDR_PRIMARY   0x76
#define BME_ADDR_SECONDARY 0x77
#define I2C_SDA            21
#define I2C_SCL            22

#define PUBLISH_MS         10000UL
#define RETRY_MS           2000UL

#define MQTT_HOST          "broker.hivemq.com"
#define MQTT_PORT          1883
#define MQTT_CLIENT_ID     "esp32-env-01"   // must be unique on the public broker

#define BASE               "ryzrr-nitjsr/env"   // unique, broker is public
#define TOPIC_DATA         BASE "/data"
#define TOPIC_STATUS       BASE "/status"
