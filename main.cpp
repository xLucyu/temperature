#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define DHTPIN 21
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define RED_LED 18
#define YELLOW_LED 17
#define GREEN_LED 19
#define BUZZER_PIN 16

const float TEMP_MIN = 22.0;
const float TEMP_MAX = 25.0;
const float HUM_MIN = 40.0;
const float HUM_MAX = 60.0;

const float TEMP_WARN_MARGIN = 0.5;  // ±0.5°C Warnbereich
const float HUM_WARN_MARGIN = 2.0;   // ±2% Warnbereich

// WLAN-Zugangsdaten
const char* ssid = "";
const char* password = "";

// Node-RED HTTP-Endpoint
const char* serverUrl = "http://172.16.76.222:1880/sensor";

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWLAN verbunden, IP: " + WiFi.localIP().toString());
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Fehler beim Lesen vom Sensor");
    delay(2000);
    return;
  }

  // LED Logik
  bool temp_critical = (temp < TEMP_MIN) || (temp > TEMP_MAX);
  bool hum_critical = (hum < HUM_MIN) || (hum > HUM_MAX);

  bool temp_warn = (!temp_critical) && ((temp >= TEMP_MIN && temp < TEMP_MIN + TEMP_WARN_MARGIN) || (temp <= TEMP_MAX && temp > TEMP_MAX - TEMP_WARN_MARGIN));
  bool hum_warn = (!hum_critical) && ((hum >= HUM_MIN && hum < HUM_MIN + HUM_WARN_MARGIN) || (hum <= HUM_MAX && hum > HUM_MAX - HUM_WARN_MARGIN));

  if (temp_critical || hum_critical) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    tone(BUZZER_PIN, 2000);
  } else if (temp_warn || hum_warn) {
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    noTone(BUZZER_PIN);
  } else {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    noTone(BUZZER_PIN);
  }

  // JSON Payload bauen
  String payload = "{\"temperature\":" + String(temp, 1) + ",\"humidity\":" + String(hum, 1) + "}";

  // HTTP POST an Node-RED senden
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    http.POST(payload);
    http.end();
  }
  delay(5000);
}
