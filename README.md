# Smart Home - Automated Monitoring and Control System

**Author:** Știrbu Loredana | **Group:** 333CB
**Platform:** Arduino Uno
**OCW Page:** https://ocw.cs.pub.ro/courses/pm/prj2026/cezar.zlatea/loredana.stirbu

## What is this project?

Imagine a home that takes care of itself. You don't need to get up to turn on the fan when it's hot, you don't need to rush outside to bring in the laundry when it starts raining, and you don't need to worry about leaving the stove on or someone breaking into your house.

That's exactly what this project does — an intelligent system built on **Arduino Uno** that constantly monitors the home environment through multiple sensors and makes automatic decisions in real time, without any input from you.

The system knows:
- 🌡️ How hot and humid it is inside
- 🌧️ Whether it's raining outside
- 💨 Whether there is dangerous gas or smoke in the air
- 👥 How many people are inside and whether an unauthorized person has entered

And it reacts automatically:
- Turns on the fan when it gets too hot
- Pulls the laundry inside when it detects rain
- Sounds the alarm and lights up LEDs when it detects gas/smoke or an intruder
- Displays all data in real time on an OLED screen

---

## How does each module work?

### 🌡️ Temperature and Humidity
An **AHT20** sensor continuously measures the temperature and humidity inside the room. If the temperature exceeds 28°C, a 360-degree servo motor (simulating a fan) starts automatically. When the temperature drops, it stops on its own. The values are permanently shown on the OLED screen.

### 👥 People Counting and Intruder Detection
Two **HC-SR04** ultrasonic sensors are placed at the entrance of the home, side by side. They work like an invisible barrier — they emit ultrasonic waves and measure the distance to objects in front of them, similar to parking sensors on a car.

The trick is in the **order** they are triggered: if the inner sensor detects movement before the outer one, it means someone is **leaving**. If the order is reversed, someone is **entering**. This way the system always knows how many people are inside.

If the number of people inside exceeds the set limit (default: 3), the system considers it an **intruder** and activates the alarm — the buzzer sounds and the red LED lights up.

### 🌧️ Rain Detection and Laundry Protection
A rain sensor detects the presence of water on its surface. At the first drop of rain, a **180-degree servo motor** automatically activates a mechanism that simulates pulling the laundry inside. When the rain stops, the servo returns to its initial position.

### 💨 Gas and Smoke Detection
The **MQ2** sensor detects dangerous concentrations of gas or smoke in the air. It works similarly to a smoke detector. When the value exceeds a calibrated threshold, the buzzer sounds and the green LED lights up as an alert signal.

### 📺 OLED Display
A **128x64 pixel OLED screen** displays all system data in real time: temperature, humidity, fan status, whether it's raining, number of people inside, gas level, and whether there is an intruder alert. The screen updates every 500ms.

---

## Components

| Component | Model | Role |
|---|---|---|
| Microcontroller | Arduino Uno (ATmega328P) | Brain of the system, processes all data |
| Display | OLED SSD1306 128x64 I2C | Shows real-time data |
| Temperature/humidity sensor | AHT20 | Measures temperature and humidity |
| Ultrasonic sensors | 2x HC-SR04 | Counts people entering/leaving |
| Rain sensor | Digital module | Detects rain |
| Gas/smoke sensor | MQ2 | Detects gas and smoke |
| Laundry servo | SG90 180° | Pulls in laundry when raining |
| Fan servo | 360° servo | Simulates cooling fan |
| Buzzer | Active buzzer 5V | Sounds alarms |
| Red LED | LED + 220Ω resistor | Visual intruder alert |
| Green LED | LED + 220Ω resistor | Visual gas/smoke alert |

---

## Libraries Used

- **U8g2lib** — for the OLED display; chosen instead of the standard Adafruit library because it uses significantly less RAM (~25 bytes vs 1024 bytes) and does not block the I2C bus, which was causing the screen to freeze
- **Adafruit_AHTX0** — official library for the AHT20 sensor, simple interface via `getEvent()`
- **Servo.h** — standard Arduino library for PWM servo control, supports both 180° and 360° servos
- **Wire.h** — manages I2C communication between Arduino, OLED (0x3C) and AHT20 (0x38)

---

## Pin Layout

| Pin | Component |
|---|---|
| D3 | Fan servo (360°) |
| D4 | Rain sensor (DO) |
| D5, D6 | HC-SR04 #2 (TRIG, ECHO) |
| D7 | Green LED |
| D8 | Red LED |
| D9 | Buzzer |
| D10 | Laundry servo (180°) |
| D11, D12 | HC-SR04 #1 (ECHO, TRIG) |
| A1 | MQ2 (analog) |
| A4, A5 | I2C bus (SDA, SCL) — OLED + AHT20 |

---

## Code Structure
loop()
├── readMainSensors()      // reads AHT20, MQ2, rain sensor
├── handlePeopleCounter()  // HC-SR04 x2, enter/exit logic
├── controlActuators()     // servos, LEDs, buzzer
├── printEverything()      // Serial Monitor every 1000ms
└── updateOLED()           // screen refresh every 500ms

---

## Key Design Decisions and Optimizations

**Problem: OLED screen freezing** — the Adafruit_SSD1306 library called on every loop iteration was overloading the shared I2C bus with the AHT20 sensor, causing the screen to lock up at the initial values.
**Solution:** Switched to **U8g2** which uses a `firstPage/nextPage` page-by-page rendering mode, drastically reducing RAM usage and eliminating I2C conflicts.

**Problem: Electrical noise on the MQ2 analog pin** — blowing warm air near the AHT20 was causing false spikes in the MQ2 gas readings due to electromagnetic interference on the A1 wire.
**Solution:** The `gasThreshold` was set well above the baseline value (~46 in clean air) to filter out false positives.

**Problem: pulseIn blocking execution** — the default 25000µs timeout on `pulseIn()` for the ultrasonic sensors was blocking the CPU for up to 50ms per loop cycle with both sensors.
**Solution:** Timeout reduced to **5000µs**, cutting the maximum blocking time to 10ms per cycle.
