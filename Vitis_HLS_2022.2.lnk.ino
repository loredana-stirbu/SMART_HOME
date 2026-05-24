#include <Wire.h>
#include <Servo.h>
#include <U8g2lib.h>
#include <Adafruit_AHTX0.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
Adafruit_AHTX0 aht;

Servo clothesServo;
Servo fanServo;

#define TRIG1 12
#define ECHO1 11
#define TRIG2 5
#define ECHO2 6

#define RAIN_PIN 4
#define GAS_PIN A1

#define SERVO_CLOTHES 10
#define SERVO_FAN 3

#define BUZZER 9
#define LED_RED 8
#define LED_GREEN 7

const int distanceThreshold = 13;
const int gasThreshold = 62;
const float tempThreshold = 28.0;
const int maxPeople = 3;

int peopleCount = 0;
int firstSensor = 0;

unsigned long firstSensorTime = 0;
unsigned long lastCountTime = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastOLEDUpdate = 0;

const unsigned long sequenceTimeout = 1500;
const unsigned long countCooldown = 1200;

bool ahtOK = false;

float temperature = 0;
float humidity = 0;
int gasValue = 0;

bool rainDetected = false;
bool highTemp = false;
bool gasDetected = false;
bool intruderAlert = false;

long d1Global = 999;
long d2Global = 999;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  pinMode(RAIN_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  clothesServo.attach(SERVO_CLOTHES);
  fanServo.attach(SERVO_FAN);

  clothesServo.write(90);
  fanServo.write(90);

  Serial.println(F("Pornire sistem Smart Home..."));

  oled.begin();
  oled.setFont(u8g2_font_5x7_tf);

  oled.firstPage();
  do {
    oled.setCursor(0, 10);
    oled.print(F("SMART HOME"));
    oled.setCursor(0, 25);
    oled.print(F("Sistem pornit"));
  } while (oled.nextPage());

  if (aht.begin()) {
    ahtOK = true;
    Serial.println(F("AHT20 gasit la 0x38"));
  } else {
    Serial.println(F("AHT20 NU a fost gasit"));
  }

  delay(1000);
}

void loop() {
  readMainSensors();
  handlePeopleCounter();
  controlActuators();

  if (millis() - lastSerialPrint > 1000) {
    printEverything();
    lastSerialPrint = millis();
  }

  if (millis() - lastOLEDUpdate > 500) {
    updateOLED();
    lastOLEDUpdate = millis();
  }
}

void readMainSensors() {
  if (ahtOK) {
    sensors_event_t humEvent, tempEvent;
    aht.getEvent(&humEvent, &tempEvent);

    temperature = tempEvent.temperature;
    humidity = humEvent.relative_humidity;
  }

  gasValue = analogRead(GAS_PIN);

  // Daca la tine ploaia apare invers, schimba LOW cu HIGH
  rainDetected = digitalRead(RAIN_PIN) == LOW;

  highTemp = temperature > tempThreshold;
  gasDetected = gasValue > gasThreshold;
  intruderAlert = peopleCount > maxPeople;
}

void controlActuators() {
  if (highTemp) {
    fanServo.write(180);
  } else {
    fanServo.write(90);
  }

  if (rainDetected) {
    clothesServo.write(180);
  } else {
    clothesServo.write(90);
  }

  digitalWrite(LED_RED, intruderAlert ? HIGH : LOW);
  digitalWrite(LED_GREEN, gasDetected ? HIGH : LOW);

  if (intruderAlert || gasDetected) {
    digitalWrite(BUZZER, HIGH);
  } else {
    digitalWrite(BUZZER, LOW);
  }
}

long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 25000);

  if (duration == 0) {
    return 999;
  }

  return duration * 0.034 / 2;
}

void handlePeopleCounter() {
  d1Global = readDistanceCM(TRIG1, ECHO1);
  delay(40);
  d2Global = readDistanceCM(TRIG2, ECHO2);

  bool sensor1Active = d1Global < distanceThreshold;
  bool sensor2Active = d2Global < distanceThreshold;

  unsigned long now = millis();

  if (now - lastCountTime < countCooldown) {
    return;
  }

  if (firstSensor == 0) {
    if (sensor1Active && !sensor2Active) {
      firstSensor = 1;
      firstSensorTime = now;
      Serial.println(F("MISCARE: senzorul 1 primul"));
    }

    if (sensor2Active && !sensor1Active) {
      firstSensor = 2;
      firstSensorTime = now;
      Serial.println(F("MISCARE: senzorul 2 primul"));
    }
  }

  if (firstSensor == 1 && sensor2Active) {
    peopleCount++;
    Serial.println(F("REZULTAT: persoana A INTRAT"));
    resetCounter();
    lastCountTime = now;
  }

  if (firstSensor == 2 && sensor1Active) {
    peopleCount--;

    if (peopleCount < 0) {
      peopleCount = 0;
    }

    Serial.println(F("REZULTAT: persoana A IESIT"));
    resetCounter();
    lastCountTime = now;
  }

  if (firstSensor != 0 && now - firstSensorTime > sequenceTimeout) {
    Serial.println(F("MISCARE: secventa anulata"));
    resetCounter();
  }
}

void resetCounter() {
  firstSensor = 0;
  firstSensorTime = 0;
}

void updateOLED() {
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_5x7_tf);

    oled.setCursor(0, 7);
    oled.print(F("Temp: "));
    oled.print(temperature, 1);
    oled.print(F(" C"));

    oled.setCursor(0, 16);
    oled.print(F("Umid: "));
    oled.print(humidity, 1);
    oled.print(F(" %"));

    oled.setCursor(0, 25);
    oled.print(F("Fan: "));
    oled.print(highTemp ? F("ON") : F("OFF"));

    oled.setCursor(0, 34);
    oled.print(F("Ploaie: "));
    oled.print(rainDetected ? F("DA") : F("NU"));

    oled.setCursor(0, 43);
    oled.print(F("Oameni: "));
    oled.print(peopleCount);

    oled.setCursor(0, 52);
    oled.print(F("Gaz: "));
    oled.print(gasDetected ? F("ALERTA") : F("OK"));

    oled.setCursor(0, 61);
    oled.print(F("Intrus: "));
    oled.print(intruderAlert ? F("DA") : F("NU"));

  } while (oled.nextPage());
}

void printEverything() {
  Serial.println(F("========== STATUS =========="));

  Serial.print(F("Distanta senzor 1: "));
  Serial.print(d1Global);
  Serial.println(F(" cm"));

  Serial.print(F("Distanta senzor 2: "));
  Serial.print(d2Global);
  Serial.println(F(" cm"));

  Serial.print(F("Persoane in casa: "));
  Serial.println(peopleCount);

  Serial.print(F("Temperatura: "));
  Serial.print(temperature);
  Serial.println(F(" C"));

  Serial.print(F("Umiditate: "));
  Serial.print(humidity);
  Serial.println(F(" %"));

  Serial.print(F("Ventilator: "));
  Serial.println(highTemp ? F("ON") : F("OFF"));

  Serial.print(F("Gaz/fum valoare: "));
  Serial.println(gasValue);

  Serial.print(F("Alerta gaz/fum: "));
  Serial.println(gasDetected ? F("DA") : F("NU"));

  Serial.print(F("Ploaie detectata: "));
  Serial.println(rainDetected ? F("DA") : F("NU"));

  Serial.print(F("Intrus: "));
  Serial.println(intruderAlert ? F("DA") : F("NU"));

  Serial.print(F("Buzzer: "));
  Serial.println((intruderAlert || gasDetected) ? F("ON") : F("OFF"));

  Serial.println(F("============================"));
}
