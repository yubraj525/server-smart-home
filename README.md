# ESP IoT Smart Home

Complete plug-and-play home automation stack:

- ESP32 firmware using Arduino, WiFi, WebSockets, and JSON.
- Node.js Express backend using `ws`.
- React Vite dashboard with live sensor charts, assistant messages, device controls, and dynamic devices.

## Configuration

### ESP32 Pin Mapping

Edit `firmware/smarthome_esp32/smarthome_esp32.ino`.

```cpp
const uint8_t LIGHT_RELAY_PIN = 16;
const uint8_t FAN_RELAY_PIN = 17;
const uint8_t DOOR_RELAY_PIN = 18;
const uint8_t AC_RELAY_PIN = 19;

const uint8_t DHT_PIN = 4;
const uint8_t GAS_SENSOR_PIN = 34;
const uint8_t MOISTURE_SENSOR_PIN = 35;
const uint8_t FIRE_SENSOR_PIN = 32;
```

Set WiFi and backend address:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* WS_HOST = "192.168.1.10";
const uint16_t WS_PORT = 4000;
const char* WS_PATH = "/ws?role=esp";
```

### Backend Config

Edit `backend/config.js` or use environment variables.

```js
port: Number(process.env.PORT || 4000)
websocketPath: "/ws"
```

Backend URLs:

- HTTP status: `http://localhost:4000/status`
- WebSocket: `ws://localhost:4000/ws`

### Frontend Config

Edit `frontend/src/config.js` or set Vite environment variables.

```js
VITE_API_BASE_URL=http://localhost:4000
VITE_WS_URL=ws://localhost:4000/ws?role=dashboard
```

## JSON Protocol

ESP32 to server:

```json
{
  "type": "sensor_update",
  "temperature": 30,
  "humidity": 55,
  "gas": 120,
  "moisture": 40,
  "fire": 150
}
```

Server to ESP32:

```json
{
  "type": "device_control",
  "device": "light",
  "state": "on"
}
```

Dashboard to server:

```json
{
  "type": "device_control",
  "device": "fan",
  "state": "off"
}
```

## Setup

1. Install Node.js 20 or newer.
2. Install dependencies:

```bash
npm install
npm --prefix frontend install
```

3. Start backend and dashboard:

```bash
npm run dev
```

4. Open the dashboard:

```text
http://localhost:5173
```

5. Open Arduino IDE or PlatformIO and install these libraries:

- `ArduinoJson`
- `WebSockets` by Markus Sattler
- `DHT sensor library`

6. Update `WIFI_SSID`, `WIFI_PASSWORD`, and `WS_HOST` in the firmware.
7. Upload `firmware/smarthome_esp32/smarthome_esp32.ino` to the ESP32.

## HTTP Status

`GET /status` returns:

```json
{
  "devices": {
    "light": "off",
    "fan": "off",
    "door": "off",
    "ac": "off"
  },
  "sensors": {
    "temperature": 30,
    "humidity": 55,
    "gas": 120,
    "moisture": 40,
    "fire": 150
  }
}
```
