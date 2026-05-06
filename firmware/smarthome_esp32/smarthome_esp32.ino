#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

// =========================
// WiFi and backend config
// =========================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* WS_HOST = "192.168.1.10";  // Backend machine IP address.
const uint16_t WS_PORT = 4000;
const char* WS_PATH = "/ws?role=esp";

// =========================
// ESP32 pin mapping
// =========================
const uint8_t LIGHT_RELAY_PIN = 16;
const uint8_t FAN_RELAY_PIN = 17;
const uint8_t DOOR_RELAY_PIN = 18;
const uint8_t AC_RELAY_PIN = 19;

const uint8_t DHT_PIN = 4;
const uint8_t GAS_SENSOR_PIN = 34;
const uint8_t MOISTURE_SENSOR_PIN = 35;
const uint8_t FIRE_SENSOR_PIN = 32;

// Change this if your relay board is active-high.
const uint8_t RELAY_ON = LOW;
const uint8_t RELAY_OFF = HIGH;

// =========================
// Sensor and reporting config
// =========================
#define DHT_TYPE DHT22
const unsigned long SENSOR_READ_INTERVAL_MS = 2000;
const float TEMPERATURE_THRESHOLD_C = 1.0;
const float HUMIDITY_THRESHOLD_PERCENT = 3.0;
const int GAS_THRESHOLD = 25;
const int MOISTURE_THRESHOLD = 3;
const int FIRE_THRESHOLD = 25;

struct SensorData {
  float temperature;
  float humidity;
  int gas;
  int moisture;
  int fire;
};

DHT dht(DHT_PIN, DHT_TYPE);
WebSocketsClient webSocket;

SensorData lastSent = {NAN, NAN, -1, -1, -1};
unsigned long lastSensorReadAt = 0;

void connectWiFi();
void connectWebSocket();
SensorData readSensors();
void handleIncomingMessage(const String& payload);
void applyDeviceState(const char* device, const char* state);
void sendSensorUpdate(const SensorData& data);
bool shouldSendSensorUpdate(const SensorData& data);

void setup() {
  Serial.begin(115200);

  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(DOOR_RELAY_PIN, OUTPUT);
  pinMode(AC_RELAY_PIN, OUTPUT);

  digitalWrite(LIGHT_RELAY_PIN, RELAY_OFF);
  digitalWrite(FAN_RELAY_PIN, RELAY_OFF);
  digitalWrite(DOOR_RELAY_PIN, RELAY_OFF);
  digitalWrite(AC_RELAY_PIN, RELAY_OFF);

  dht.begin();
  connectWiFi();
  connectWebSocket();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  webSocket.loop();

  const unsigned long now = millis();
  if (now - lastSensorReadAt >= SENSOR_READ_INTERVAL_MS) {
    lastSensorReadAt = now;
    SensorData data = readSensors();

    if (shouldSendSensorUpdate(data)) {
      sendSensorUpdate(data);
      lastSent = data;
    }
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void connectWebSocket() {
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(15000, 3000, 2);

  webSocket.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_CONNECTED:
        Serial.println("WebSocket connected.");
        break;
      case WStype_TEXT:
        {
          String message;
          message.reserve(length);
          for (size_t i = 0; i < length; i++) {
            message += (char)payload[i];
          }
          handleIncomingMessage(message);
        }
        break;
      case WStype_DISCONNECTED:
        Serial.println("WebSocket disconnected. Reconnecting automatically.");
        break;
      default:
        break;
    }
  });
}

SensorData readSensors() {
  SensorData data;

  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    data.temperature = isnan(lastSent.temperature) ? 0.0 : lastSent.temperature;
  } else {
    data.temperature = temperature;
  }

  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    data.humidity = isnan(lastSent.humidity) ? 0.0 : lastSent.humidity;
  } else {
    data.humidity = humidity;
  }

  data.gas = analogRead(GAS_SENSOR_PIN);

  // Convert raw capacitive moisture reading into a rough 0-100 percentage.
  const int rawMoisture = analogRead(MOISTURE_SENSOR_PIN);
  data.moisture = constrain(map(rawMoisture, 4095, 1200, 0, 100), 0, 100);
  data.fire = analogRead(FIRE_SENSOR_PIN);

  return data;
}

bool shouldSendSensorUpdate(const SensorData& data) {
  if (isnan(lastSent.temperature)) {
    return true;
  }

  const bool temperatureChanged = fabs(data.temperature - lastSent.temperature) > TEMPERATURE_THRESHOLD_C;
  const bool humidityChanged = fabs(data.humidity - lastSent.humidity) > HUMIDITY_THRESHOLD_PERCENT;
  const bool gasChanged = abs(data.gas - lastSent.gas) > GAS_THRESHOLD;
  const bool moistureChanged = abs(data.moisture - lastSent.moisture) > MOISTURE_THRESHOLD;
  const bool fireChanged = abs(data.fire - lastSent.fire) > FIRE_THRESHOLD;

  return temperatureChanged || humidityChanged || gasChanged || moistureChanged || fireChanged;
}

void sendSensorUpdate(const SensorData& data) {
  StaticJsonDocument<256> doc;
  doc["type"] = "sensor_update";
  doc["temperature"] = roundf(data.temperature * 10.0) / 10.0;
  doc["humidity"] = roundf(data.humidity * 10.0) / 10.0;
  doc["gas"] = data.gas;
  doc["moisture"] = data.moisture;
  doc["fire"] = data.fire;

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);

  Serial.print("Sensor update sent: ");
  Serial.println(output);
}

void handleIncomingMessage(const String& payload) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Invalid JSON from server: ");
    Serial.println(error.c_str());
    return;
  }

  const char* type = doc["type"] | "";

  if (strcmp(type, "device_control") == 0) {
    const char* device = doc["device"] | "";
    const char* state = doc["state"] | "";
    applyDeviceState(device, state);
  }
}

void applyDeviceState(const char* device, const char* state) {
  const uint8_t relayState = strcmp(state, "on") == 0 ? RELAY_ON : RELAY_OFF;

  if (strcmp(device, "light") == 0) {
    digitalWrite(LIGHT_RELAY_PIN, relayState);
  } else if (strcmp(device, "fan") == 0) {
    digitalWrite(FAN_RELAY_PIN, relayState);
  } else if (strcmp(device, "door") == 0) {
    digitalWrite(DOOR_RELAY_PIN, relayState);
  } else if (strcmp(device, "ac") == 0) {
    digitalWrite(AC_RELAY_PIN, relayState);
  } else {
    Serial.print("Unknown device command ignored: ");
    Serial.println(device);
    return;
  }

  Serial.print("Device updated: ");
  Serial.print(device);
  Serial.print(" -> ");
  Serial.println(state);
}
