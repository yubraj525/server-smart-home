/*
 * ============================================================
 *  SmartHome ESP32 Firmware
 *  Architecture: ESP32 <-> WebSocket Backend <-> React Dashboard
 * ============================================================
 *  Required libraries (install via Arduino Library Manager):
 *    - ArduinoJson        by Benoit Blanchon  (>= 6.x)
 *    - DHT sensor library by Adafruit         (+ Adafruit Unified Sensor)
 *    - WebSockets         by Markus Sattler   (>= 2.3.x)
 * ============================================================
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

// ─────────────────────────────────────────────
//  1. CONFIGURATION  — edit before flashing
// ─────────────────────────────────────────────
const char*    WIFI_SSID     = "YOUR_WIFI_SSID";
const char*    WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char*    WS_HOST       = "192.168.1.10"; // IP of PC running the Node backend
const uint16_t WS_PORT       = 4000;
const char*    WS_PATH       = "/ws?role=esp";

// ─────────────────────────────────────────────
//  2. PIN MAPPING
//     Default: relay board is ACTIVE-LOW.
//     Swap RELAY_ON/RELAY_OFF if yours is active-high.
// ─────────────────────────────────────────────
const uint8_t LIGHT1_PIN   = 16;   // Living Room
const uint8_t LIGHT2_PIN   = 17;   // Bedroom
const uint8_t LIGHT3_PIN   = 18;   // Kitchen
const uint8_t LIGHT4_PIN   = 19;   // Hall
const uint8_t FAN_PIN      = 21;
const uint8_t DOOR_PIN     = 22;
const uint8_t AC_PIN       = 23;

const uint8_t DHT_PIN      = 4;
const uint8_t GAS_PIN      = 34;   // ADC1 – input only
const uint8_t MOISTURE_PIN = 35;   // ADC1 – input only
const uint8_t FIRE_PIN     = 32;   // ADC1 – input only

const uint8_t RELAY_ON  = LOW;    // change to HIGH for active-high boards
const uint8_t RELAY_OFF = HIGH;

// ─────────────────────────────────────────────
//  3. SENSOR THRESHOLDS
// ─────────────────────────────────────────────
#define DHT_TYPE DHT22
const unsigned long SENSOR_INTERVAL_MS   = 2000;
const float  TEMP_THRESHOLD             = 1.0f;
const float  HUM_THRESHOLD             = 3.0f;
const int    GAS_THRESHOLD             = 25;
const int    MOISTURE_THRESHOLD        = 3;
const int    FIRE_THRESHOLD            = 25;

// ─────────────────────────────────────────────
//  4. GLOBALS
// ─────────────────────────────────────────────
struct SensorData {
  float temperature;
  float humidity;
  int   gas;
  int   moisture;
  int   fire;
};

DHT dht(DHT_PIN, DHT_TYPE);
WebSocketsClient webSocket;

SensorData    lastSent         = {NAN, NAN, -1, -1, -1};
unsigned long lastSensorReadAt = 0;
bool          wsConnected      = false;

// ── Forward declarations ──────────────────────
void       printBanner();
void       connectWiFi();
void       connectWebSocket();
SensorData readSensors();
bool       shouldSend(const SensorData& d);
void       sendSensorUpdate(const SensorData& d);
void       handleIncomingMessage(const String& payload);
void       applyDeviceState(const char* device, const char* state);

// ─────────────────────────────────────────────
//  setup()
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(300);

  printBanner();

  // Relay pins — all OFF at boot
  Serial.println("[INIT] Configuring relay output pins...");
  const uint8_t relayPins[] = {
    LIGHT1_PIN, LIGHT2_PIN, LIGHT3_PIN, LIGHT4_PIN,
    FAN_PIN, DOOR_PIN, AC_PIN
  };
  for (uint8_t pin : relayPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, RELAY_OFF);
  }
  Serial.println("[INIT] All relays -> OFF.");

  // DHT sensor
  Serial.println("[INIT] Starting DHT22 sensor...");
  dht.begin();
  Serial.println("[INIT] DHT22 ready.");

  // Network
  connectWiFi();
  connectWebSocket();

  Serial.println("[BOOT] Setup complete — entering loop.");
  Serial.println("────────────────────────────────────────");
}

// ─────────────────────────────────────────────
//  loop()
// ─────────────────────────────────────────────
void loop() {
  // Reconnect WiFi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Connection lost! Reconnecting...");
    wsConnected = false;
    connectWiFi();
    connectWebSocket();
  }

  // Drive WebSocket (handles incoming msgs, pings, auto-reconnect)
  webSocket.loop();

  // Read + conditionally send sensor data
  const unsigned long now = millis();
  if (now - lastSensorReadAt >= SENSOR_INTERVAL_MS) {
    lastSensorReadAt = now;
    SensorData data = readSensors();

    Serial.printf("[SENSOR] Temp=%.1fC  Hum=%.1f%%  Gas=%d  Moisture=%d  Fire=%d\n",
                  data.temperature, data.humidity,
                  data.gas, data.moisture, data.fire);

    if (shouldSend(data)) {
      sendSensorUpdate(data);
      lastSent = data;
    } else {
      Serial.println("[SENSOR] No significant change — skipping send.");
    }
  }
}

// ─────────────────────────────────────────────
//  printBanner()
// ─────────────────────────────────────────────
void printBanner() {
  Serial.println();
  Serial.println("═══════════════════════════════════════════════");
  Serial.println("  SmartHome ESP32  |  Booting up...");
  Serial.println("═══════════════════════════════════════════════");
  Serial.printf ("  WiFi SSID  : %s\n", WIFI_SSID);
  Serial.printf ("  WS Server  : ws://%s:%d%s\n", WS_HOST, WS_PORT, WS_PATH);
  Serial.println("  Devices    : light1 light2 light3 light4 fan door ac");
  Serial.println("  Sensors    : DHT22 (temp+hum)  Gas  Moisture  Fire");
  Serial.println("  Relay logic: ACTIVE-LOW (change RELAY_ON if needed)");
  Serial.println("═══════════════════════════════════════════════");
  Serial.println();
}

// ─────────────────────────────────────────────
//  connectWiFi()
// ─────────────────────────────────────────────
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("[WIFI] Starting connection...");
  Serial.printf ("[WIFI] SSID: %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempts % 20 == 0) {
      Serial.println();
      Serial.printf("[WIFI] Still connecting... (%d attempts)\n", attempts);
    }
  }

  Serial.println();
  Serial.println("[WIFI] Connected!");
  Serial.printf ("[WIFI] IP    : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf ("[WIFI] RSSI  : %d dBm\n", WiFi.RSSI());
}

// ─────────────────────────────────────────────
//  connectWebSocket()
// ─────────────────────────────────────────────
void connectWebSocket() {
  Serial.println("[WS]  Configuring WebSocket client...");
  Serial.printf ("[WS]  Target: ws://%s:%d%s\n", WS_HOST, WS_PORT, WS_PATH);

  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.setReconnectInterval(5000);      // retry every 5 s
  webSocket.enableHeartbeat(15000, 3000, 2); // ping every 15 s, 2 misses = drop

  webSocket.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {

      case WStype_CONNECTED:
        wsConnected = true;
        Serial.println("[WS]  ✓ Connected to server!");
        Serial.printf ("[WS]  Path: %s\n", WS_PATH);
        break;

      case WStype_DISCONNECTED:
        wsConnected = false;
        Serial.println("[WS]  ✗ Disconnected. Auto-retry in 5 s...");
        break;

      case WStype_TEXT: {
        String msg;
        msg.reserve(length + 1);
        for (size_t i = 0; i < length; i++) msg += (char)payload[i];
        Serial.print("[WS]  <- Received: ");
        Serial.println(msg);
        handleIncomingMessage(msg);
        break;
      }

      case WStype_PING:
        Serial.println("[WS]  <> Ping received.");
        break;

      case WStype_PONG:
        Serial.println("[WS]  <> Pong received.");
        break;

      case WStype_ERROR:
        Serial.println("[WS]  ✗ Error event.");
        break;

      default:
        break;
    }
  });

  Serial.println("[WS]  Handler registered. Waiting for connection...");
}

// ─────────────────────────────────────────────
//  readSensors()
// ─────────────────────────────────────────────
SensorData readSensors() {
  SensorData d;

  float t = dht.readTemperature();
  d.temperature = isnan(t)
    ? (isnan(lastSent.temperature) ? 0.0f : lastSent.temperature)
    : t;

  float h = dht.readHumidity();
  d.humidity = isnan(h)
    ? (isnan(lastSent.humidity) ? 0.0f : lastSent.humidity)
    : h;

  d.gas = analogRead(GAS_PIN);

  // Capacitive moisture: higher raw value = drier soil
  int raw = analogRead(MOISTURE_PIN);
  d.moisture = constrain(map(raw, 4095, 1200, 0, 100), 0, 100);

  d.fire = analogRead(FIRE_PIN);

  return d;
}

// ─────────────────────────────────────────────
//  shouldSend()
// ─────────────────────────────────────────────
bool shouldSend(const SensorData& d) {
  if (isnan(lastSent.temperature)) return true; // always send first reading

  return fabs(d.temperature - lastSent.temperature) > TEMP_THRESHOLD
      || fabs(d.humidity    - lastSent.humidity)    > HUM_THRESHOLD
      || abs(d.gas      - lastSent.gas)      > GAS_THRESHOLD
      || abs(d.moisture - lastSent.moisture) > MOISTURE_THRESHOLD
      || abs(d.fire     - lastSent.fire)     > FIRE_THRESHOLD;
}

// ─────────────────────────────────────────────
//  sendSensorUpdate()
// ─────────────────────────────────────────────
void sendSensorUpdate(const SensorData& d) {
  StaticJsonDocument<256> doc;
  doc["type"]        = "sensor_update";
  doc["temperature"] = roundf(d.temperature * 10.0f) / 10.0f;
  doc["humidity"]    = roundf(d.humidity    * 10.0f) / 10.0f;
  doc["gas"]         = d.gas;
  doc["moisture"]    = d.moisture;
  doc["fire"]        = d.fire;

  String output;
  output.reserve(200);
  serializeJson(doc, output);

  webSocket.sendTXT(output);
  Serial.print("[WS]  -> Sent: ");
  Serial.println(output);
}

// ─────────────────────────────────────────────
//  handleIncomingMessage()
// ─────────────────────────────────────────────
void handleIncomingMessage(const String& payload) {
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.print("[MSG] JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  const char* type = doc["type"] | "";

  if (strcmp(type, "device_control") == 0) {
    const char* device = doc["device"] | "";
    const char* state  = doc["state"]  | "";
    Serial.printf("[MSG] device_control: device=%s  state=%s\n", device, state);
    applyDeviceState(device, state);

  } else if (strcmp(type, "state_update") == 0) {
    // Server broadcasts full state on connect / any change — informational only
    Serial.println("[MSG] state_update received (no relay action).");

  } else {
    Serial.printf("[MSG] Unhandled type: \"%s\"\n", type);
  }
}

// ─────────────────────────────────────────────
//  applyDeviceState()
//  Device names MUST match backend + frontend exactly.
// ─────────────────────────────────────────────
void applyDeviceState(const char* device, const char* state) {
  const uint8_t relay = (strcmp(state, "on") == 0) ? RELAY_ON : RELAY_OFF;

  if      (strcmp(device, "light1") == 0) digitalWrite(LIGHT1_PIN, relay);
  else if (strcmp(device, "light2") == 0) digitalWrite(LIGHT2_PIN, relay);
  else if (strcmp(device, "light3") == 0) digitalWrite(LIGHT3_PIN, relay);
  else if (strcmp(device, "light4") == 0) digitalWrite(LIGHT4_PIN, relay);
  else if (strcmp(device, "fan")    == 0) digitalWrite(FAN_PIN,    relay);
  else if (strcmp(device, "door")   == 0) digitalWrite(DOOR_PIN,   relay);
  else if (strcmp(device, "ac")     == 0) digitalWrite(AC_PIN,     relay);
  else {
    Serial.printf("[RELAY] Unknown device ignored: \"%s\"\n", device);
    return;
  }

  Serial.printf("[RELAY] %s -> %s  (pin=%s)\n",
                device, state, (relay == RELAY_ON) ? "LOW" : "HIGH");
}
