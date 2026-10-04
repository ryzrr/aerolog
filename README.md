# AeroLog

An ESP32 environment logger.

![build](https://github.com/ryzrr/aerolog/actions/workflows/build.yml/badge.svg)

ESP32 + BME280 publishes temperature, humidity and pressure over MQTT every 10 s, with auto-reconnect, Last Will online/offline status and a Node-RED dashboard (no dashboard code).

Full spec: [docs/prd.md](docs/prd.md)

## Wiring
| BME280 | ESP32 |
| --- | --- |
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

Address is 0x76 (SDO to GND) or 0x77; both are tried at boot. Chip ID must be 0x60 (BMP280 has no humidity).

## Build (3 steps)
1. `cp include/secrets.h.example include/secrets.h` and set WiFi credentials.
2. Change `BASE` in `include/config.h` to a unique prefix (the broker is public).
3. `pio run -e esp32dev -t upload` then `pio device monitor`

## Topics
| Topic | Payload | QoS / retain |
| --- | --- | --- |
| `<BASE>/data` | `{"temp_c":27.4,"humidity":61.2,"pressure_hpa":1008.3}` every 10 s (`null` for NaN) | 0 / no |
| `<BASE>/status` | `online` / `offline` (Last Will) | online 0 retained; offline 1 retained |

## Requirements to code to test
| FR | Where | Test |
| --- | --- | --- |
| FR1 | `main.cpp`, `sensor.cpp` | 1 |
| FR2 | `payload.cpp`, `main.cpp` | 2, `pio test -e native` |
| FR3 | `net.cpp` `netLoop()` | 3 |
| FR4 | `net.cpp` | 2, 4 |
| FR5 | Node-RED: `mqtt in` > `json` > 3 `change` > 3 gauges + chart | dashboard |
| FR6 | `main.cpp` `setup()` | 5 |

Unit tests: `pio test -e native`.

## Known limitations
Public broker, no TLS or auth; QoS 0 data; no OTA or deep sleep.
