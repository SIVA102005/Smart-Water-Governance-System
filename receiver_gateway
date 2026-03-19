/**************************************************************
 GATEWAY NODE
 -------------------------------------------------------------
 - Receives LoRa data from multiple houses
 - Sends data to ThingSpeak (Cloud)
**************************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <LoRa.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

// ThingSpeak API
String apiKey = "YOUR_API_KEY";

// LoRa Pins
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26

void setup() {
  Serial.begin(115200);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // Initialize LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Failed!");
    while (1);
  }

  Serial.println("Gateway Ready");
}

void loop() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String receivedData = "";

    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.println("Received: " + receivedData);

    sendToCloud(receivedData);
  }
}

// Send to ThingSpeak
void sendToCloud(String data) {

  HTTPClient http;

  String url = "http://api.thingspeak.com/update?api_key=" + apiKey +
               "&field1=" + data;

  http.begin(url);
  http.GET();
  http.end();
}
