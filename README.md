# AeroLog

ESP32 firmware that reads a BME280 sensor and publishes temperature, humidity and pressure as JSON over MQTT every 10 seconds.

![build](https://github.com/ryzrr/aerolog/actions/workflows/build.yml/badge.svg)

## What is in this repo

- Firmware for the ESP32 (Arduino framework, built with PlatformIO)
- BME280 reading over I2C, with a fallback from address `0x76` to `0x77`
- JSON payload formatting, where an invalid reading (NaN) becomes `null`
- WiFi and MQTT connection code with retry logic and an MQTT Last Will message for `online` / `offline` status
- Unit tests for the payload code, which run on a PC
- A GitHub Actions workflow that runs the tests and builds the firmware
- The original requirements document in [docs/prd.md](docs/prd.md)

Not included: a dashboard. The PRD describes a Node-RED dashboard, but no flow has been built or added here.

## Status

- The firmware compiles for `esp32dev`.
- The 4 payload unit tests pass on a PC.
- The firmware has not been run on real hardware yet, so the sensor reading, reconnect and Last Will behaviour are written but untested.

## Hardware

| Part | Qty |
| --- | --- |
| ESP32 DevKit (30-pin) | 1 |
| BME280 I2C module (chip ID `0x60`) | 1 |
| Breadboard and 4 jumper wires | 1 |

| BME280 pin | ESP32 pin |
| --- | --- |
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

A BMP280 (chip ID `0x58`) has no humidity sensor, so use a BME280.

## Build and run

You need [PlatformIO](https://platformio.org/install).

1. Create your secrets file and fill in your WiFi details (it is gitignored):
   ```bash
   cp include/secrets.h.example include/secrets.h
   ```
2. Change `BASE` in `include/config.h` to a unique topic prefix. The default broker, `broker.hivemq.com`, is public.
3. Upload and open the serial monitor:
   ```bash
   pio run -e esp32dev -t upload
   pio device monitor
   ```

Run the unit tests (no hardware needed):

```bash
pio test -e native
```

## MQTT topics

| Topic | Payload | QoS / retain |
| --- | --- | --- |
| `<BASE>/data` | JSON reading every 10 s | 0 / no |
| `<BASE>/status` | `online` on connect, `offline` via Last Will | online: 0, retained. offline: 1, retained |

Example data payload:

```json
{"temp_c":27.4,"humidity":61.2,"pressure_hpa":1008.3}
```

## Project layout

```text
├── platformio.ini            board, libraries, native test env
├── include/
│   ├── config.h              pins, interval, topics, client ID
│   └── secrets.h.example     copy to secrets.h (gitignored)
├── src/
│   ├── main.cpp              setup() and loop()
│   ├── sensor.h / .cpp       BME280 wrapper
│   ├── payload.h / .cpp      Reading to JSON, no hardware code
│   └── net.h / .cpp          WiFi and MQTT, reconnect, Last Will
├── test/test_payload/        payload unit tests
├── docs/prd.md               requirements and design notes
└── .github/workflows/        CI workflow
```

## Known limitations

- The public broker has no TLS or authentication, so anyone can read or publish to a known topic.
- Data is sent at QoS 0.
- WiFi credentials are compiled into the firmware.
