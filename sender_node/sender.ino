/**************************************************************
 SMART WATER NODE (HOUSE UNIT)
 -------------------------------------------------------------
 - Measures water usage using flow sensor
 - Controls solenoid valve using relay
 - Detects leakage
 - Uses RTC for daily reset
 - Sends data via LoRa
**************************************************************/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <RTClib.h>

// ---------------- PIN CONFIGURATION ----------------
#define FLOW_SENSOR_PIN 34    // Flow sensor input
#define RELAY_PIN 27          // Controls valve
#define BUTTON_PIN 4          // Manual stop button
#define BUZZER_PIN 15         // Leak alert buzzer
#define LED_PIN 2             // Leak indicator LED

// LoRa Pins
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26

// ---------------- GLOBAL VARIABLES ----------------
RTC_DS3231 rtc;

volatile int pulseCount = 0;   // Flow pulses
float flowRate = 0.0;          // L/min
float totalLitres = 0.0;       // Total usage

// Family configuration
int familyMembers = 4;         // CHANGE per house
float perPersonLimit = 55.0;   // Jal Jeevan Mission
float dailyLimit;

// Carry forward
float remaining = 0;

// Valve state
bool valveOpen = true;

// Leak detection
unsigned long leakStartTime = 0;
bool leakDetected = false;

// Calibration factor (depends on sensor)
float calibrationFactor = 4.5;

// ---------------- INTERRUPT FUNCTION ----------------
void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  // Pin modes
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Initially valve open
  digitalWrite(RELAY_PIN, HIGH);

  // Attach flow sensor interrupt
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), countPulse, FALLING);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC NOT FOUND");
    while (1);
  }

  // Set initial daily limit
  dailyLimit = familyMembers * perPersonLimit;

  // Initialize LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Failed!");
    while (1);
  }

  Serial.println("SYSTEM READY");
}

// ---------------- MAIN LOOP ----------------
void loop() {

  /************ 1. FLOW CALCULATION ************/
  pulseCount = 0;
  delay(1000);  // measure for 1 second

  flowRate = pulseCount / calibrationFactor;
  totalLitres += (flowRate / 60.0);

  Serial.print("Flow Rate: ");
  Serial.print(flowRate);
  Serial.print(" L/min | Used: ");
  Serial.println(totalLitres);

  /************ 2. MANUAL BUTTON ************/
  if (digitalRead(BUTTON_PIN) == LOW) {
    valveOpen = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Manual Valve Closed");
  }

  /************ 3. AUTO LIMIT CONTROL ************/
  if (totalLitres >= dailyLimit) {
    valveOpen = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Limit Reached");
  }

  /************ 4. LEAK DETECTION ************/
  if (!valveOpen && flowRate > 0.5) {

    if (leakStartTime == 0)
      leakStartTime = millis();

    if (millis() - leakStartTime > 300000) { // 5 minutes
      leakDetected = true;

      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);

      Serial.println("LEAK DETECTED!");
    }

  } else {
    leakStartTime = 0;
    leakDetected = false;

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  /************ 5. RTC DAILY RESET ************/
  DateTime now = rtc.now();

  if (now.hour() == 0 && now.minute() == 0) {

    remaining = dailyLimit - totalLitres;
    if (remaining < 0) remaining = 0;

    dailyLimit = (familyMembers * perPersonLimit) + remaining;

    totalLitres = 0;
    valveOpen = true;
    digitalWrite(RELAY_PIN, HIGH);

    Serial.println("NEW DAY RESET");

    delay(60000); // avoid multiple resets
  }

  /************ 6. SEND DATA VIA LORA ************/
  LoRa.beginPacket();
  LoRa.print("USED:");
  LoRa.print(totalLitres);
  LoRa.print(",REMAIN:");
  LoRa.print(dailyLimit - totalLitres);
  LoRa.print(",LEAK:");
  LoRa.print(leakDetected);
  LoRa.endPacket();

  delay(2000);
}
