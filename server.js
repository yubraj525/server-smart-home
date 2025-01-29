const express = require("express");
const WebSocket = require("ws");
const cors = require("cors");
const http = require("http");
const bodyParser = require("body-parser");
const { default: axios } = require("axios");

const app = express();
const port = 5000;
const esp32Url = "http://192.168.1.101:8080/update"; // Replace with ESP32 IP address

app.use(cors());
app.use(bodyParser.json());

// In-memory data storage
let sensorData = {
  temperature: 34.5,
  humidity: 15,
  gasLevel: 15,
  airQuality: 85,
  fireDetected: false,
};

let houseStatus = {
  doors: [
    { name: "Bedroom", locked: false },
    { name: "Kitchen", locked: false },
    { name: "Hall", locked: false },
    { name: "Gate", locked: false },
  ],
  lights: [
    { name: "Bedroom", on: false },
    { name: "Kitchen", on: false },
    { name: "Hall", on: false },
  ],
};

let controls = [
  { name: "Light", isOn: true },
  { name: "Fan", isOn: false },
  { name: "Fridge", isOn: true },
  { name: "Coffee", isOn: false },
  { name: "TV", isOn: false },
  { name: "Wi-Fi", isOn: true },
  { name: "Power", isOn: true },
  { name: "Lock", isOn: true },
];

let energyUsage = [
  { day: "Mon", value: 20 },
  { day: "Tue", value: 25 },
  { day: "Wed", value: 30 },
  { day: "Thu", value: 22 },
  { day: "Fri", value: 28 },
  { day: "Sat", value: 35 },
  { day: "Sun", value: 32 },
];

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let clients = [];

// WebSocket connection handler
wss.on("connection", (ws) => {
  clients.push(ws);
  console.log("New WebSocket connection established");

  ws.on("close", () => {
    clients = clients.filter((client) => client !== ws);
    console.log("WebSocket connection closed");
  });

  // Send initial data to the new client
  sendInitialData(ws);

  // Handle incoming WebSocket messages
  ws.on("message", (message) => {
    try {
      handleWebSocketMessage(ws, message);
    } catch (error) {
      console.error("Error handling WebSocket message:", error);
    }
  });
});

// Express Routes
app.get("/sensor-data", (req, res) => res.json(sensorData));
app.post("/sensor-data", handleSensorDataPost);

app.get("/house-status", (req, res) => res.json(houseStatus));
app.post("/house-status", handleHouseStatusPost);

app.get("/controls", (req, res) => res.json(controls));
app.post("/controls", handleControlsPost);

app.get("/esp-update", handleESP32Update);

// Helper Functions
function sendInitialData(ws) {
  const initialData = { sensorData, houseStatus, controls, energyUsage };
  ws.send(JSON.stringify(initialData));
}

function handleWebSocketMessage(ws, message) {
  const data = JSON.parse(message);
  console.log("Received data:", data);

  let updateData = {};

  if (data.doors || data.lights) {
    updateHouseStatus(data);
    updateData = { houseStatus };
  } else if (data.name && "isOn" in data) {
    updateControlState(data);
    updateData = { controls };
  } else if (data.day && "value" in data) {
    updateEnergyUsage(data);
    updateData = { energyUsage };
  } else if (data.sensorData) {
    updateSensorData(data.sensorData);
    updateData = { sensorData };
  }

  if (Object.keys(updateData).length > 0) {
    broadcastUpdate(updateData);
  }

  notifyESP32(data);
}

function updateHouseStatus(data) {
  if (data.doors) {
    data.doors.forEach((updatedDoor) => {
      const door = houseStatus.doors.find((d) => d.name === updatedDoor.name);
      if (door) door.locked = updatedDoor.locked;
    });
  }

  if (data.lights) {
    data.lights.forEach((updatedLight) => {
      const light = houseStatus.lights.find(
        (l) => l.name === updatedLight.name
      );
      if (light) light.on = updatedLight.on;
    });
  }
}

function updateControlState(data) {
  const control = controls.find((c) => c.name === data.name);
  if (control) control.isOn = data.isOn;
}

function updateEnergyUsage(data) {
  const usage = energyUsage.find((u) => u.day === data.day);
  if (usage) usage.value = data.value;
}

function updateSensorData(newSensorData) {
  sensorData = { ...sensorData, ...newSensorData };
}

function broadcastUpdate(updateData) {
  const updateMessage = JSON.stringify(updateData);
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(updateMessage);
    }
  });
}
async function notifyESP32(update) {
  const params = {};

  if (update.doors) {
    update.doors.forEach((door) => {
      params[`d${door.name}`] = door.locked; // Example: dHall=true
    });
  }

  if (update.lights) {
    update.lights.forEach((light) => {
      params[`l${light.name}`] = light.on; // Example: lHall=false
    });
  }

  try {
    const response = await axios.post(esp32Url, update, {
      // params, // Dynamically constructed query string
    });
    console.log("ESP32 Response:", response.data);
    broadcastUpdate(update);
  } catch (error) {
    console.error("Failed to notify ESP32:", error);
  }
}

function handleSensorDataPost(req, res) {
  const newSensorData = req.body;
  updateSensorData(newSensorData);

  broadcastUpdate({ sensorData });
  res.status(200).send("Sensor data updated");
}

function handleHouseStatusPost(req, res) {
  const { doors, lights } = req.body;

  if (doors) {
    updateHouseStatus({ doors });
  }
  if (lights) {
    updateHouseStatus({ lights });
  }

  broadcastUpdate({ houseStatus });
  res.status(200).send("House status updated");
}

function handleControlsPost(req, res) {
  const { name, isOn } = req.body;
  updateControlState({ name, isOn });

  broadcastUpdate({ controls });
  res.status(200).send("Control updated");
}

function handleESP32Update(req, res) {
  const updates = req.query;
  console.log("Received ESP32 updates:", updates);

  try {
    // Handle sensor data updates
    if (updates.temperature) sensorData.temperature = parseFloat(updates.temperature);
    if (updates.humidity) sensorData.humidity = parseFloat(updates.humidity);
    if (updates.gasLevel) sensorData.gasLevel = parseFloat(updates.gasLevel);
    if (updates.airQuality) sensorData.airQuality = parseFloat(updates.airQuality);
    if (updates.fireDetected) sensorData.fireDetected = Boolean(parseInt(updates.fireDetected));

    // Handle individual light updates
    let lightsChanged = false;
    const lightUpdates = [];
    
    if (updates.hallLight !== undefined) {
      const light = houseStatus.lights.find(l => l.name === "Hall");
      if (light) {
        light.on = Boolean(parseInt(updates.hallLight));
        lightUpdates.push({ name: "Hall", on: light.on });
        lightsChanged = true;
      }
    }
    if (updates.kitchenLight !== undefined) {
      const light = houseStatus.lights.find(l => l.name === "Kitchen");
      if (light) {
        light.on = Boolean(parseInt(updates.kitchenLight));
        lightUpdates.push({ name: "Kitchen", on: light.on });
        lightsChanged = true;
      }
    }
    if (updates.bedroomLight !== undefined) {
      const light = houseStatus.lights.find(l => l.name === "Bedroom");
      if (light) {
        light.on = Boolean(parseInt(updates.bedroomLight));
        lightUpdates.push({ name: "Bedroom", on: light.on });
        lightsChanged = true;
      }
    }

    // Handle individual door updates
    let doorsChanged = false;
    const doorUpdates = [];

    if (updates.mainDoor !== undefined) {
      const door = houseStatus.doors.find(d => d.name === "FrontDoor");
      if (door) {
        door.locked = Boolean(parseInt(updates.mainDoor));
        doorUpdates.push({ name: "FrontDoor", locked: door.locked });
        doorsChanged = true;
      }
    }
    if (updates.bedroomDoor !== undefined) {
      const door = houseStatus.doors.find(d => d.name === "Bedroom");
      if (door) {
        door.locked = Boolean(parseInt(updates.bedroomDoor));
        doorUpdates.push({ name: "Bedroom", locked: door.locked });
        doorsChanged = true;
      }
    }
    if (updates.kitchenDoor !== undefined) {
      const door = houseStatus.doors.find(d => d.name === "Kitchen");
      if (door) {
        door.locked = Boolean(parseInt(updates.kitchenDoor));
        doorUpdates.push({ name: "Kitchen", locked: door.locked });
        doorsChanged = true;
      }
    }

    // Prepare response
    const response = {
      status: "success",
      timestamp: new Date().toISOString(),
      updates: {
        sensorData: { ...sensorData },
        lights: lightUpdates,
        doors: doorUpdates
      }
    };

    // Broadcast updates to all connected clients
    if (lightsChanged || doorsChanged) {
      broadcastUpdate({ 
        sensorData, 
        houseStatus
      });
    }

    res.status(200).json(response);
  } catch (error) {
    console.error("Error processing ESP32 update:", error);
    res.status(400).json({
      status: "error",
      message: error.message,
      timestamp: new Date().toISOString()
    });
  }
}

// Start the server
server.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
