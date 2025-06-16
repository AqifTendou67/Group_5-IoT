#define BLYNK_TEMPLATE_ID "TMPL6z0y3uFSQ"
#define BLYNK_TEMPLATE_NAME "IoT Wildfire Detection System"
#define BLYNK_AUTH_TOKEN "0AjzdLnADEyTKt9FZ8oWFsHkgFUfFqPh"

#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include "DHT.h"

// Blynk Auth Token
char auth[] = BLYNK_AUTH_TOKEN;

// Wi-Fi credentials
const char* ssid = "P18@unifi";
const char* password = "FSDK2024";

// Telegram
const char* botToken = "7554301904:AAFm77Ce-VjVujHUDGsPD6ysAmaQoimu47g";
const char* chatID = "858040743";

// Pins
#define DHTPIN 32
#define DHTTYPE DHT11
const int flamePin = 5;
const int buzzerPin = 15;

DHT dht(DHTPIN, DHTTYPE);
float alarmThreshold = 30.0;

// Timer for alerts
unsigned long lastAlertTime = 0;
const unsigned long alertInterval = 5000;

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  pinMode(flamePin, INPUT);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected");

  // Start Blynk
  Blynk.begin(auth, ssid, password);
}

// 🧠 Blynk-compatible URL encoder
String urlEncode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

void loop() {
  Blynk.run();

  float temperatureC = dht.readTemperature();

  // Flame debounce
  bool flameDetected = false;
  int flameCount = 0;
  for (int i = 0; i < 5; i++) {
    if (digitalRead(flamePin) == LOW) flameCount++;
    delay(5);
  }
  if (flameCount >= 3) flameDetected = true;

  Serial.print("🌡️ Temp: ");
  Serial.print(temperatureC);
  Serial.print(" °C | 🔥 Flame: ");
  Serial.println(flameDetected ? "YES" : "NO");

  if (flameDetected) {
    tone(buzzerPin, 2000);
  } else {
    noTone(buzzerPin);
  }

  if (flameDetected && (millis() - lastAlertTime > alertInterval)) {
    String alertMsg = "🔥 Wildfire detected!\n🌡️ Temp: " + String(temperatureC, 1) + "°C";
    sendTelegramAlert(alertMsg);
    lastAlertTime = millis();
  }

  Blynk.virtualWrite(V0, temperatureC);
  Blynk.virtualWrite(V1, flameDetected ? 1 : 0);

  delay(1000);
}

// 🔽 Place this **after** loop()
void sendTelegramAlert(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = "https://api.telegram.org/bot" + String(botToken) +
                 "/sendMessage?chat_id=" + String(chatID) +
                 "&text=" + urlEncode(message);

    https.begin(client, url);
    int responseCode = https.GET();
    if (responseCode > 0) {
      Serial.println("✅ Telegram alert sent.");
      Serial.println("Response: " + https.getString());
    } else {
      Serial.print("❌ Error sending alert. Code: ");
      Serial.println(responseCode);
    }
    https.end();
  } else {
    Serial.println("❌ WiFi not connected.");
  }
}