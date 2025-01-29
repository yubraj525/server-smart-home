#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>


const char* ssid = "scifi_2.4";
const char* password = "88888888";
const char* serverUrl = "http://192.168.1.74:5000/esp-update";  // Replace with your Express server URL

WebServer server(8080);

// Pins
const int hallLightPin = 4;
const int bedroomLightPin = 2;
const int kitchenLightPin = 3;
const int garageLightPin = 6;

const int mainDoorPin = 5;
const int bedroomDoorPin = 7;
const int kitchenDoorPin = 8;
const int garageDoorPin = 9;



// Additional options
const int lockPin = 10;
const int fanPin = 11;
const int wifiPin = 12;
const int coffeePin = 13;
const int powerPin = 14;
const int fridgePin = 15;
const int choiceTwoPin = 16;
// Sensor variables
float temperature = 0.0;
float humidity = 0.0;
int gasLevel = 0;
int airQuality = 0;
bool fireDetected = false;

int hallLightState = -1;
int mainDoorState = -1;
int bedroomLightState = -1;
int kitchenLightState = -1;
int garageLightState = -1;
int bedroomDoorState = -1;
int kitchenDoorState = -1;
int garageDoorState = -1;
int lockState = -1;
int fanState = -1;
int wifiState = -1;
int coffeeState = -1;
int powerState = -1;
int fridgeState = -1;
int choiceTwoState = -1;

// Previous values to detect changes
float prevTemperature = -1.0;
float prevHumidity = -1.0;
int prevGasLevel = -1;
int prevAirQuality = -1;
bool prevFireDetected = false;
int prevMainDoorState = -1;
int prevHallLightState = -1;
int prevBedroomLightState = -1;
int prevChoiceTwoState = -1;
int prevWifiState = -1;
int prevCoffeeState = -1;
int prevGarageLightState = -1;
int prevBedroomDoorState = -1;
int prevKitchenDoorState = -1;
int prevGarageDoorState = -1;
int prevLockState = -1;
int prevFanState = -1;
int prevPowerState = -1;
int prevFridgeState = -1;
int prevKitchenLightState = -1;



struct Door {
  String name;
  bool locked;
};

struct Light {
  String name;
  bool on;
};

std::vector<Door> doors;
std::vector<Light> lights;

void setup() {
  Serial.begin(9600);
  Serial.println("init");
  pinMode(hallLightPin, OUTPUT);
  pinMode(mainDoorPin, OUTPUT);
  Serial.println("100");

  // pinMode(bedroomLightPin, OUTPUT);
  // pinMode(kitchenLightPin, OUTPUT);
  // pinMode(garageLightPin, OUTPUT);
  // Serial.println("200");

  // pinMode(bedroomDoorPin, OUTPUT);
  // Serial.println("300--");
  // pinMode(kitchenDoorPin, OUTPUT);

  // Serial.println("400");
  // pinMode(garageDoorPin, OUTPUT);
  // Serial.println("500");


  // pinMode(lockPin, OUTPUT);
  // pinMode(fanPin, OUTPUT);
  // pinMode(wifiPin, OUTPUT);
  // pinMode(coffeePin, OUTPUT);
  // pinMode(powerPin, OUTPUT);
  // pinMode(fridgePin, OUTPUT);
  // pinMode(choiceTwoPin, OUTPUT);

  Serial.println("1");

  // Initialize relays to default state
  digitalWrite(hallLightPin, LOW);  // Light off
  digitalWrite(mainDoorPin, LOW);   // Door locked
  // digitalWrite(bedroomLightPin, LOW);
  // digitalWrite(kitchenLightPin, LOW);
  // digitalWrite(garageLightPin, LOW);
  // digitalWrite(bedroomDoorPin, LOW);
  // digitalWrite(kitchenDoorPin, LOW);
  // digitalWrite(garageDoorPin, LOW);
  // digitalWrite(lockPin, LOW);
  // digitalWrite(fanPin, LOW);
  // digitalWrite(wifiPin, LOW);
  // digitalWrite(coffeePin, LOW);
  // digitalWrite(powerPin, LOW);
  // digitalWrite(fridgePin, LOW);
  // digitalWrite(choiceTwoPin, LOW);
  Serial.println("2");




  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime >= 10000) {  // 10 seconds timeout
      Serial.println("Failed to connect to WiFi");
      break;
    }
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" ");
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.on("/initial", HTTP_GET, []() {
    /// send all data in json format
  });
  // Define endpoint to receive updates
  server.on("/update", HTTP_POST, handleUpdate);



  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient();
  // Simulate sensor data (replace with actual sensor readings)
  // temperature = 10;  // Replace with your temperature sensor logic
  humidity = 34.0;    // Replace with your humidity sensor logic
  gasLevel = 76.0;    // Replace with gas sensor logic
  airQuality = 56.0;  // Replace with air quality sensor logic
  fireDetected = false;
  temperature = 22.0;  // Read one byte from the serial buffer
  hallLightState = 1;
  bool dataChanged = false;  // Flag to track if any value has changed

  // Check and update temperature
  if (temperature != prevTemperature) {
    temperature = temperature;
    Serial.println("Temperature changed");
    dataChanged = true;
  }

  // Check and update humidity
  if (humidity != prevHumidity) {
    humidity = humidity;
    Serial.println("Humidity changed");
    dataChanged = true;
  }

  // Check and update gas level
  if (gasLevel != prevGasLevel) {

    gasLevel = gasLevel;
    Serial.println("Gas Level changed");
    dataChanged = true;
  }

  // Check and update air quality
  if (airQuality != prevAirQuality) {

    airQuality = airQuality;
    Serial.println("Air Quality changed");
    dataChanged = true;
  }

  // Check and update fire detection
  if (fireDetected != prevFireDetected) {

    fireDetected = fireDetected;
    Serial.println("Fire detection status changed");
    dataChanged = true;
  }

  // Check and update hall light state
  if (hallLightState != prevHallLightState) {

    hallLightState = hallLightState;
    Serial.println("Hall Light State changed");
    dataChanged = true;
  }

  // Check and update main door state
  if (mainDoorState != prevMainDoorState) {

    mainDoorState = mainDoorState;
    Serial.println("Main Door State changed");
    dataChanged = true;
  }

  // Check and update bedroom light state
  if (bedroomLightState != prevBedroomLightState) {

    bedroomLightState = bedroomLightState;
    Serial.println("Bedroom Light State changed");
    dataChanged = true;
  }

  // Check and update kitchen light state
  if (kitchenLightState != prevKitchenLightState) {

    kitchenLightState = kitchenLightState;
    Serial.println("Kitchen Light State changed");
    dataChanged = true;
  }

  // Check and update garage light state
  if (garageLightState != prevGarageLightState) {

    garageLightState = garageLightState;
    Serial.println("Garage Light State changed");
    dataChanged = true;
  }

  // Check and update bedroom door state
  if (bedroomDoorState != prevBedroomDoorState) {

    bedroomDoorState = bedroomDoorState;
    Serial.println("Bedroom Door State changed");
    dataChanged = true;
  }

  // Check and update kitchen door state
  if (kitchenDoorState != prevKitchenDoorState) {

    kitchenDoorState = kitchenDoorState;
    Serial.println("Kitchen Door State changed");
    dataChanged = true;
  }

  // Check and update garage door state
  if (garageDoorState != prevGarageDoorState) {

    garageDoorState = garageDoorState;
    Serial.println("Garage Door State changed");
    dataChanged = true;
  }

  // Check and update lock state
  if (lockState != prevLockState) {

    lockState = lockState;
    Serial.println("Lock State changed");
    dataChanged = true;
  }

  // Check and update fan state
  if (fanState != prevFanState) {

    fanState = fanState;
    Serial.println("Fan State changed");
    dataChanged = true;
  }

  // Check and update WiFi state
  if (wifiState != prevWifiState) {

    wifiState = wifiState;
    Serial.println("WiFi State changed");
    dataChanged = true;
  }

  // Check and update coffee state
  if (coffeeState != prevCoffeeState) {

    coffeeState = coffeeState;
    Serial.println("Coffee State changed");
    dataChanged = true;
  }

  // Check and update power state
  if (powerState != prevPowerState) {

    powerState = powerState;
    Serial.println("Power State changed");
    dataChanged = true;
  }

  // Check and update fridge state
  if (fridgeState != prevFridgeState) {

    fridgeState = fridgeState;
    Serial.println("Fridge State changed");
    dataChanged = true;
  }

  // Check and update choice two state
  if (choiceTwoState != prevChoiceTwoState) {

    choiceTwoState = choiceTwoState;
    Serial.println("Choice Two State changed");
    dataChanged = true;
  }

  // If any data changed, send updated data to the server
  if (dataChanged) {
    sendSensorData();
  }
}


void sendSensorData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = serverUrl;
    url += "?";  //temperature=" + String(temperature);


    // Add changed sensor data to the URL
    if (temperature != prevTemperature) {
      Serial.println(temperature);
      prevTemperature = temperature;
      url += "temperature=" + String(temperature) + "&";
    }
    if (humidity != prevHumidity) {
      prevHumidity = humidity;
      url += "humidity=" + String(humidity) + "&";
    }
    if (gasLevel != prevGasLevel) {
      prevGasLevel = gasLevel;
      url += "gasLevel=" + String(gasLevel) + "&";
    }
    if (airQuality != prevAirQuality) {
      prevAirQuality = airQuality;
      url += "airQuality=" + String(airQuality) + "&";
    }
    if (fireDetected != prevFireDetected) {
      prevFireDetected = fireDetected;
      url += "fireDetected=" + String(fireDetected) + "&";
    }
    if (hallLightState != prevHallLightState) {
      prevHallLightState = hallLightState;
      url += "hallLight=" + String(hallLightState) + "&";
    }
    if (mainDoorState != prevMainDoorState) {
      prevMainDoorState = mainDoorState;
      url += "mainDoor=" + String(mainDoorState) + "&";
    }
    if (bedroomLightState != prevBedroomLightState) {
      prevBedroomLightState = bedroomLightState;
      url += "bedroomLight=" + String(bedroomLightState) + "&";
    }
    if (kitchenLightState != prevKitchenLightState) {
      prevKitchenLightState = kitchenLightState;
      url += "kitchenLight=" + String(kitchenLightState) + "&";
    }
    if (garageLightState != prevGarageLightState) {
      prevGarageLightState = garageLightState;
      url += "garageLight=" + String(garageLightState) + "&";
    }
    if (bedroomDoorState != prevBedroomDoorState) {
      prevBedroomDoorState = bedroomDoorState;
      url += "bedroomDoor=" + String(bedroomDoorState) + "&";
    }
    if (kitchenDoorState != prevKitchenDoorState) {
      prevKitchenDoorState = kitchenDoorState;
      url += "kitchenDoor=" + String(kitchenDoorState) + "&";
    }
    if (garageDoorState != prevGarageDoorState) {
      prevGarageDoorState = garageDoorState;
      url += "garageDoor=" + String(garageDoorState) + "&";
    }
    if (lockState != prevLockState) {
      prevLockState = lockState;
      url += "lockState=" + String(lockState) + "&";
    }
    if (fanState != prevFanState) {
      prevFanState = fanState;
      url += "fanState=" + String(fanState) + "&";
    }
    if (wifiState != prevWifiState) {
      prevWifiState = wifiState;
      url += "wifiState=" + String(wifiState) + "&";
    }
    if (coffeeState != prevCoffeeState) {
      prevCoffeeState = coffeeState;
      url += "coffeeState=" + String(coffeeState) + "&";
    }
    if (powerState != prevPowerState) {
      prevPowerState = powerState;
      url += "powerState=" + String(powerState) + "&";
    }
    if (fridgeState != prevFridgeState) {
      prevFridgeState = fridgeState;
      url += "fridgeState=" + String(fridgeState) + "&";
    }
    if (choiceTwoState != prevChoiceTwoState) {
      prevChoiceTwoState = choiceTwoState;
      url += "choiceTwoState=" + String(choiceTwoState) + "&";
    }
    Serial.println(url);
    Serial.println("-----");


    // Remove the trailing "&" if it exists
    if (url.endsWith("&")) {
      url = url.substring(0, url.length() - 1);
    }
    Serial.println(url);

    // Send the GET request
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("Server response: ");
      Serial.println(httpResponseCode);
      Serial.println(http.getString());
    } else {
      Serial.print("Error sending GET request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}



void handleUpdate() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }

    JsonObject updates = doc["updates"];
    bool dataChanged = false;

    // Process door updates
    if (updates.containsKey("doors")) {
      JsonArray doorsArray = updates["doors"];
      doors.clear();
      for (JsonObject door : doorsArray) {
        Door d;
        d.name = door["name"].as<String>();
        d.locked = door["locked"];
        doors.push_back(d);
        // Update physical pins based on door status
        updateDoorPin(d.name, d.locked);
        dataChanged = true;
      }
    }

    // Process light updates
    if (updates.containsKey("lights")) {
      JsonArray lightsArray = updates["lights"];
      lights.clear();
      for (JsonObject light : lightsArray) {
        Light l;
        l.name = light["name"].as<String>();
        l.on = light["on"];
        lights.push_back(l);
        // Update physical pins based on light status
        updateLightPin(l.name, l.on);
        dataChanged = true;
      }
    }

    // Send success response
    DynamicJsonDocument response(256);
    response["status"] = "success";
    response["message"] = "Updates applied";
    String responseJson;
    serializeJson(response, responseJson);
    server.send(200, "application/json", responseJson);

    // If data changed, send updates back to server
    if (dataChanged) {
      sendSensorData();
    }
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
  }
}