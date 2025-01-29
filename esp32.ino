#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <DHT.h>

// Define sensor pins

#define DHTTYPE DHT11    // Define sensor type
#define GAS_SENSOR_PIN 34  // Gas Sensor
#define MOISTURE_SENSOR_PIN 32  // Moisture Sensor
#define DHT11_SENSOR_PIN 33  // DHT11 Temperature & Humidity Sensor

DHT dht(DHTPIN, DHTTYPE);  // Initialize DHT sensor

const char* ssid = "scifi_2.4";
const char* password = "88888888";
const char* serverUrl = "https://smartmate-api.aasishkarki.com.np/esp-update";  // Replace with your Express server URL

WebServer server(8080);

// Pins
// Define output pins
const int hallLightPin = 13;
const int bedroomLightPin = 14;
const int kitchenLightPin = 16;
const int garageLightPin = 17;

const int frontDoorPin = 18;
const int bedroomDoorPin = 19;
const int kitchenDoorPin = 21;
const int garageDoorPin = 22;

// Additional options
const int lockPin = 23;
const int fanPin = 25;
const int wifiPin = 26;
const int coffeePin = 27;
const int powerPin = 2;
const int fridgePin = 4;
const int choiceTwoPin = 5;
// Sensor variables
float temperature = 0.0;
float humidity = 0.0;
int gasLevel = 0;
int airQuality = 0;
bool fireDetected = false;

int hallLightState = -1;
int frontDoorState = -1;
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
int prevfrontDoorState = -1;
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
    dht.begin();  // Start DHT11 sensor
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN , INPUT);
  pinMode(34, INPUT);


  pinMode(hallLightPin, OUTPUT);
  pinMode(bedroomDoorPin, OUTPUT);
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
  //  for(int j=2;j<17;j++){
  //   pinMode(j, OUTPUT);
  // }

  // for(int i=2;i<17;i++){
  //    digitalWrite(i, LOW);
  // }

  // Serial.println("1");

  // Initialize relays to default state
  // digitalWrite(hallLightPin, LOW);    // Light off
  // digitalWrite(bedroomDoorPin, LOW);  // Door locked

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
humidity = dht.readHumidity();   // Replace with your humidity sensor logic
  gasLevel =  analogRead(GAS_SENSOR_PIN);   // Replace with gas sensor logic
  airQuality = 56.0;  // Replace with air quality sensor logic
  fireDetected = true;
temperature = dht.readTemperature();  // Read one byte from the serial buffer
  hallLightState = digitalRead(hallLightPin);
  bedroomDoorState = digitalRead(bedroomDoorPin);

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
  if (frontDoorState != prevfrontDoorState) {

    frontDoorState = frontDoorState;
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
    if (frontDoorState != prevfrontDoorState) {
      prevfrontDoorState = frontDoorState;
      url += "mainDoor=" + String(frontDoorState) + "&";
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
  // Create a JSON document to store the incoming data
  DynamicJsonDocument doc(1024);

  // Parse the incoming JSON request
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println(" ");
    Serial.println(body);

    // Deserialize the JSON data
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      // If there's an error in deserialization, send a response indicating failure
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON format\"}");
      return;
    }

    // Check if the data is related to lights or doors
    if (doc.containsKey("lights")) {
      // Handle lights data
      JsonArray lights = doc["lights"].as<JsonArray>();

      for (JsonObject light : lights) {
        String lightName = light["name"];
        bool on = light["on"];

        // if (lightName == "Bedroom") {
        //   digitalWrite(bedroomLightPin, on ? HIGH : LOW);
        //   bedroomLightState = on ? 1 : 0;
        // } else if (lightName == "Kitchen") {
        //   digitalWrite(kitchenLightPin, on ? HIGH : LOW);
        //   kitchenLightState = on ? 1 : 0;
        // } else
        if (lightName == "Hall") {

          digitalWrite(hallLightPin, on ? HIGH : LOW);
          hallLightState = on ? 1 : 0;
          Serial.println(hallLightState);
        }
      }
    } else if (doc.containsKey("doors")) {
      // Handle doors data
      JsonArray doors = doc["doors"].as<JsonArray>();

      for (JsonObject door : doors) {
        String doorName = door["name"];
        bool locked = door["locked"];

        if (doorName == "Bedroom") {
          digitalWrite(bedroomDoorPin, locked ? HIGH : LOW);
          bedroomDoorState = locked ? 1 : 0;  // Update the state for Bedroom door
          Serial.println(bedroomDoorState);
        }
        //  else if (doorName == "Kitchen") {
        //   kitchenDoorState = locked ? 1 : 0; // Update the state for Kitchen door
        // } else if (doorName == "Hall") {
        //   hallDoorState = locked ? 1 : 0; // Update the state for Hall door
        // } else
        //  if (doorName == "Gate") {
        //   frontDoorState = locked ? 1 : 0; // Update the state for FrontDoor
        // }
      }
    } else {
      // If neither "lights" nor "doors" is present in the JSON, send an error
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No lights or doors data found\"}");
      return;
    }

    // Send a success response after processing the data
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    // If no data was provided
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data provided\"}");
  }
}
