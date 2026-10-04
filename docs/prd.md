# ESP32 Environment Logger: PRD + TRD

Oct 4, 2026 · @nikhil

## PRD 1: Overview

Ek chhota ESP32 device jo BME280 se temperature, humidity aur pressure padhke har 10 second mein MQTT pe bhejta hai, aur ek live dashboard use dikhata hai.

**Kyun:** Amazon SDE-I Intern form ke liye ek embedded project chahiye jo ek evening mein chal jaaye aur ek hafte mein poora samajh aa jaaye: code, circuit, protocol aur design choices.

**Size ka rule:** firmware chhota rahega, aur dashboard ke liye koi code nahi likhna.

| Limit | Target |
| --- | --- |
| Firmware | v1: 1 file, 46 lines; repo mein modules mein split (TRD 4) |
| Libraries | 2 (Adafruit BME280, PubSubClient) |
| Dashboard / backend code | 0 lines (public broker + Node-RED config) |
| Hardware cost | \~₹800-1,150 |
| Time | 1 evening to run, 1 day to understand every line |

## PRD 2: Scope aur success criteria

Sirf wahi cheezein scope mein hain jo ek chhote firmware se ho jaati hain.

| In scope | Out of scope (jaan-boojhkar nahi kiya) |
| --- | --- |
| BME280 se 3 readings (I2C) | FreeRTOS tasks |
| WiFi + MQTT publish, 10 s interval | TLS / AWS IoT Core |
| Auto-reconnect (WiFi aur broker) | Custom web dashboard / backend |
| Online/offline status (Last Will) | Deep sleep, OTA updates, PCB |
| Node-RED dashboard (config only) | Data storage / database |

**Success criteria:** project "done" tab hai jab ye teeno sach hon.

- Device 30 minute tak bina reset ke har \~10 s pe reading bhejta hai.
- WiFi band karke 30 s baad on karne par data khud wapas aa jaata hai.
- Tum bina dekhe samjha sako ki har line kya karti hai.

## PRD 3: Functional requirements

Chhe requirements hain, aur har ek ka ek seedha test hai.

| ID | Requirement | Kaise check karein |
| --- | --- | --- |
| FR1 | BME280 se temperature (°C), humidity (%RH), pressure (hPa) har 10 s pe padho | Serial Monitor mein values; temperature kisi thermometer se ±2 °C ke andar |
| FR2 | Har reading ek JSON message mein topic `<base>/data` pe publish ho | MQTTX mein har \~10 s pe message dikhe |
| FR3 | WiFi aur broker gir jaaye toh device bina reset ke wapas connect ho | Hotspot 30 s band karo, phir on karo; data resume ho |
| FR4 | Connect hone par retained `online`; device achanak gire toh broker `offline` publish kare (Last Will) | ESP32 ki power kaato; `<base>/status` \~25 s mein `offline` ho |
| FR5 | Dashboard latest values aur live chart dikhaye | Node-RED dashboard pe 3 gauges + 1 chart |
| FR6 | Boot par sensor na mile toh error print karke ruk jaaye | SDA wire nikaalo; Serial pe `BME280 not found` |

## TRD 1: Architecture aur hardware

Data ek hi direction mein behta hai: sensor se ESP32, ESP32 se broker, broker se dashboard.

&#91;embedded content: data flow · 4 components, 3 links\]

ESP32 box hi tumhara apna kaam hai; baaki teen hisse ready-made hain.

**Parts list**

| Part | Qty | Approx ₹ | Note |
| --- | --- | --- | --- |
| ESP32 DevKit (30-pin) | 1 | 450-600 | Data-capable USB cable chahiye |
| BME280 I2C module | 1 | 250-400 | Chip ID `0x60` hona chahiye; `0x58` = BMP280, humidity nahi |
| Breadboard + 4 jumper wires | 1 | 100-150 |  |

**Wiring (4 wires)**

| BME280 pin | ESP32 pin |
| --- | --- |
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

I2C address `0x76` hai (SDO GND pe) ya `0x77`; agar sensor na mile toh dusra address try karo.

## TRD 2: Firmware (46 lines)

Poora firmware ek file mein hai, aur `delay()` sirf reconnect ke waqt aata hai, readings ke beech nahi.

| Decision | Kyun |
| --- | --- |
| `millis()`, `delay()` nahi | Loop block nahi hota, isliye reconnect aur `mqtt.loop()` chalte rehte hain |
| `snprintf`, ArduinoJson nahi | Ek library kam aur code chhota |
| Public HiveMQ broker, port 1883 | Account ya setup nahi; data public hota hai, isliye unique topic prefix rakho |
| Data QoS 0, status retained | PubSubClient sirf QoS 0 publish karta hai; retained status se naya subscriber turant state dekhta hai |
| Ek file, RTOS nahi | Size ka rule |

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_BME280.h>

#define BASE "ryzrr-nitjsr/env"   // unique rakho, broker public hai
const char* ssid = "YOUR_WIFI";
const char* pass = "YOUR_PASS";

WiFiClient net;
PubSubClient mqtt(net);
Adafruit_BME280 bme;
unsigned long lastSend = 0;

void connectMqtt() {
  while (!mqtt.connected()) {
    // Last Will: device achanak gire toh broker "offline" publish karega (FR4)
    if (!mqtt.connect("esp32-env-01", nullptr, nullptr, BASE "/status", 1, true, "offline")) delay(2000);
  }
  mqtt.publish(BASE "/status", "online", true);
}

void setup() {
  Serial.begin(115200);
  if (!bme.begin(0x76)) {                  // FR6: sensor na mile toh ruk jao
    Serial.println("BME280 not found");
    while (true) delay(1000);
  }
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  mqtt.setServer("broker.hivemq.com", 1883);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { WiFi.reconnect(); delay(1000); return; }  // FR3
  if (!mqtt.connected()) connectMqtt();                                         // FR3
  mqtt.loop();

  if (millis() - lastSend >= 10000) {      // FR1: har 10 s, bina delay() ke
    lastSend = millis();
    char msg[96];
    snprintf(msg, sizeof msg, "{\"temp_c\":%.1f,\"humidity\":%.1f,\"pressure_hpa\":%.1f}",
             bme.readTemperature(), bme.readHumidity(), bme.readPressure() / 100.0F);
    mqtt.publish(BASE "/data", msg);       // FR2
    Serial.println(msg);
  }
}
```

**Setup:** Arduino IDE mein board `ESP32 Dev Module`, libraries `Adafruit BME280 Library` aur `PubSubClient` (Library Manager se), baud 115200. `ssid` aur `pass` GitHub pe push mat karna.

## TRD 3: MQTT spec aur test plan

Do topics hain, aur data ek chhota JSON object hai.

| Topic | Payload | QoS / retain |
| --- | --- | --- |
| `ryzrr-nitjsr/env/data` | JSON, har 10 s | 0 / no |
| `ryzrr-nitjsr/env/status` | `online` (connect par), `offline` (Last Will) | online: 0 / retained; offline: 1 / retained |

Payload fields: `temp_c` (°C), `humidity` (%RH), `pressure_hpa` (hPa).

```json
{"temp_c":27.4,"humidity":61.2,"pressure_hpa":1008.3}
```

**Dashboard (FR5):** Node-RED mein `mqtt in` (broker.hivemq.com, topic `ryzrr-nitjsr/env/data`) se `json` node, phir 3 `change` nodes (har ek ek field `msg.payload` mein nikaalta hai), phir 3 gauges aur 1 chart. Sab nodes jodne hain, code nahi likhna; Node-RED ka dashboard package install karna padega.

**Test plan**

| # | Test | Kaise | Pass |
| --- | --- | --- | --- |
| 1 | Sensor | Serial Monitor kholo | 3 values aayein; humidity `nan` ho toh module BMP280 hai |
| 2 | Publish | MQTTX mein `ryzrr-nitjsr/env/#` subscribe karo | `data` har \~10 s, `status` = `online` |
| 3 | WiFi drop | Hotspot 30 s band, phir on | Data khud resume |
| 4 | Last Will | ESP32 ki power nikaalo | `status` \~25 s mein `offline` |
| 5 | Sensor missing | SDA wire nikaalo, reset karo | Serial pe `BME280 not found` |
| 6 | Soak | 30 minute chalne do | Koi reset ya missed message nahi |

## TRD 4: Code structure aur codebase guidelines

TRD 2 ka code v1 hai: ek file, samajhne ke liye. Jab wo chal jaaye, use neeche ke structure mein baanto, taaki repo embedded project bhi lage aur SDE project bhi.

**Repo layout**

```text
esp32-env-logger/
├── platformio.ini               # board, libraries, native test env
├── README.md                    # pitch, demo, wiring, topics, how to run
├── .gitignore                   # secrets.h, .pio/
├── include/
│   ├── config.h                 # pins, interval, topics (no secrets)
│   └── secrets.h.example        # copy to secrets.h (gitignored)
├── src/
│   ├── main.cpp                 # setup() + loop() only
│   ├── sensor.h / .cpp          # BME280 wrapper
│   ├── payload.h / .cpp         # Reading -> JSON (pure, no hardware)
│   └── net.h / .cpp             # WiFi + MQTT, reconnect, Last Will
├── test/
│   └── test_payload/test_main.cpp   # runs on PC: pio test -e native
├── docs/                        # circuit photo, dashboard screenshot, this PRD/TRD
└── .github/workflows/build.yml  # CI: pio run on every push
```

**Module contracts.** Same logic, bas files mein baanta hua; headers ke saath lagbhag 80-90 lines, aur har file ek hi kaam karti hai.

| File | Ek line ka kaam | Public API | Hardware chahiye? |
| --- | --- | --- | --- |
| `main.cpp` | Modules ko jodna, koi logic nahi | `setup()`, `loop()` | Haan |
| `sensor` | BME280 padhna | `bool sensorBegin()`, `Reading sensorRead()` | Haan (I2C) |
| `payload` | Reading ko JSON string banana | `size_t formatPayload(char* buf, size_t n, const Reading& r)` | Nahi, PC pe test hota hai |
| `net` | WiFi + MQTT connect, reconnect, Last Will, publish | `void netBegin()`, `void netLoop()`, `bool netPublish(const char* topic, const char* msg)` | Haan (WiFi) |

```cpp
// payload.h: Reading yahin rehta hai, taaki payload hardware se independent rahe
struct Reading { float temp_c, humidity, pressure_hpa; };
size_t formatPayload(char* buf, size_t n, const Reading& r);
```

`main.cpp` itna patla rehna chahiye:

```cpp
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
```

**Codebase guidelines**

| # | Guideline | Repo mein kaise dikhta hai | Priority |
| --- | --- | --- | --- |
| 1 | Hardware aur logic alag | `payload` pure function hai; `main.cpp` mein sirf jodne ka kaam | Must |
| 2 | Secrets repo mein nahi | `secrets.h` gitignored, `secrets.h.example` committed | Must |
| 3 | Magic numbers nahi | `0x76`, `PUBLISH_MS`, topic names, client ID sab `config.h` mein | Must |
| 4 | Non-blocking loop | `millis()` timer; `delay()` sirf boot aur reconnect mein | Must |
| 5 | Har failure ka check aur log | Init/connect ka return check; Serial pe `[INFO]`, `[WARN]`, `[ERR]` prefix | Must |
| 6 | Requirement se code tak traceability | Comments mein FR IDs (FR1-FR6); README mein FR to test table | Must |
| 7 | Ek naming style | Functions `camelCase`, constants `UPPER_SNAKE`, files lowercase | Must |
| 8 | Chhote, meaningful commits | `feat:`, `fix:`, `docs:`, `refactor:` prefix; pehle ek-file v1 commit, phir `refactor: split into modules`; tag `v1.0` | Must |
| 9 | README = project ka front door | Pitch line, demo GIF, architecture diagram (is doc se), wiring photo, parts list, 3-step build, topic table, test results, known limitations | Must |
| 10 | Unit test | `test_payload`: normal reading, negative temperature, NaN ka `null` banna (BMP280 ya sensor error pe bhi JSON valid rahe); `pio test -e native` | Nice |
| 11 | CI | GitHub Actions: `cp include/secrets.h.example include/secrets.h`, phir `pio run`; README mein build badge | Nice |

Must wale kaam refactor ke saath ek shaam mein ho jaate hain; Nice wale tabhi karo jab baaki sab ho chuka ho. Git history khud dikhati hai ki pehle chalaya, phir saaf kiya, jo interview mein process ka achha saboot hai.

## Build plan aur interview points

Project do dinon mein chal jaata hai; teesra din samajhne ke liye hai.

1. **Evening 1 (2-3 ghante):** wiring, code upload, Serial pe readings, MQTTX se verify (tests 1-2).
2. **Day 2 (2-3 ghante):** reconnect aur Last Will tests (3-5), Node-RED dashboard, module split (TRD 4), aur GitHub repo: README, circuit photo, demo video.
3. **Day 3:** har line bina dekhe explain karne ki practice, aur neeche ke sawaal.

Hardware abhi nahi aaya toh Wokwi (ESP32 simulator) pe tests 1-2 ho jaate hain; resume ya interview mein batao ki simulation tha.

**Optional add-ons (v1 pakka hone ke baad hi):** TLS (port 8883), NTP timestamp, exponential backoff, FreeRTOS tasks.

**Resume ke baare mein:** form ki deadline kal 10 AM hai (5 Oct 2026), aur ye project tab tak nahi banega. Ise resume mein tabhi likho jab ye chal jaaye; form mein wahi daalo jo tumne sach mein banaya hai.

**Resume bullet (build ke baad):**

> Built an ESP32 environment logger: BME280 over I2C, JSON telemetry over MQTT every 10 s, auto-reconnect with Last Will online/offline status, and a live Node-RED dashboard (46-line firmware).

**Interview mein ye puchha jayega**

| Sawaal | Ek line ka jawab |
| --- | --- |
| I2C kya hai? | 2 wires (SDA data, SCL clock), har device ka apna address; BME280 ka `0x76` ya `0x77` |
| HTTP ki jagah MQTT kyun? | Halka publish/subscribe protocol, chhote devices pe kam overhead |
| Last Will kya hai? | Connect ke waqt broker ko diya gaya message, jo device achanak gire toh broker publish karta hai |
| `delay()` ki jagah `millis()` kyun? | `delay()` poora loop rok deta hai; `millis()` se reconnect aur `mqtt.loop()` chalte rehte hain |
| Retained message kya hai? | Broker last message yaad rakhta hai, naya subscriber turant dekh leta hai |
| Public broker ka risk? | Koi bhi topic padh ya likh sakta hai; agla step TLS aur authentication |
