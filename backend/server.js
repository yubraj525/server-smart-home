const http = require("http");
const express = require("express");
const cors = require("cors");
const { WebSocketServer, WebSocket } = require("ws");
const config = require("./config");

const VALID_DEVICES = new Set(["light", "fan", "door", "ac"]);
const VALID_STATES = new Set(["on", "off"]);

const app = express();
app.use(cors({ origin: config.allowedOrigin }));
app.use(express.json());

const server = http.createServer(app);
const wss = new WebSocketServer({ server, path: config.websocketPath });

const clients = {
  esp: new Set(),
  dashboard: new Set(),
};

const state = {
  devices: {
    light: "off",
    fan: "off",
    door: "off",
    ac: "off",
  },
  sensors: {
    temperature: null,
    humidity: null,
    gas: null,
    moisture: null,
    fire: null,
  },
  connected: {
    espClients: 0,
    dashboardClients: 0,
  },
  updatedAt: null,
};

function snapshot() {
  return {
    devices: { ...state.devices },
    sensors: { ...state.sensors },
    connected: { ...state.connected },
    updatedAt: state.updatedAt,
  };
}

function sendJson(ws, payload) {
  if (ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(payload));
  }
}

function broadcastJson(targets, payload) {
  for (const client of targets) {
    sendJson(client, payload);
  }
}

function broadcastState(reason) {
  broadcastJson(clients.dashboard, {
    type: "state_update",
    reason,
    ...snapshot(),
  });
}

function sendDeviceCommand(device, deviceState) {
  const command = {
    type: "device_control",
    device,
    state: deviceState,
  };

  broadcastJson(clients.esp, command);
}

function parseClientRole(requestUrl) {
  const url = new URL(requestUrl, "http://localhost");
  const role = url.searchParams.get("role");
  return role === "esp" ? "esp" : "dashboard";
}

function updateConnectionCounts() {
  state.connected.espClients = clients.esp.size;
  state.connected.dashboardClients = clients.dashboard.size;
}

function isFiniteNumber(value) {
  return typeof value === "number" && Number.isFinite(value);
}

function handleSensorUpdate(message) {
  const nextSensors = {};

  if (isFiniteNumber(message.temperature)) {
    nextSensors.temperature = message.temperature;
  }
  if (isFiniteNumber(message.humidity)) {
    nextSensors.humidity = message.humidity;
  }
  if (isFiniteNumber(message.gas)) {
    nextSensors.gas = message.gas;
  }
  if (isFiniteNumber(message.moisture)) {
    nextSensors.moisture = message.moisture;
  }
  if (isFiniteNumber(message.fire)) {
    nextSensors.fire = message.fire;
  }

  state.sensors = {
    ...state.sensors,
    ...nextSensors,
  };
  state.updatedAt = new Date().toISOString();

  broadcastJson(clients.dashboard, {
    type: "sensor_update",
    sensors: { ...state.sensors },
    updatedAt: state.updatedAt,
  });
}

function handleDeviceControl(message, ws) {
  const { device, state: desiredState } = message;

  if (!VALID_DEVICES.has(device) && !(device in state.devices)) {
    sendJson(ws, {
      type: "error",
      message: `Unknown device: ${device}`,
    });
    return;
  }

  if (!VALID_STATES.has(desiredState)) {
    sendJson(ws, {
      type: "error",
      message: `Invalid state for ${device}: ${desiredState}`,
    });
    return;
  }

  state.devices[device] = desiredState;
  state.updatedAt = new Date().toISOString();

  sendDeviceCommand(device, desiredState);
  broadcastState("device_control");
}

function handleAddDevice(message, ws) {
  const rawName = String(message.device || "").trim().toLowerCase();
  const device = rawName.replace(/[^a-z0-9_-]/g, "_").slice(0, 24);

  if (!device) {
    sendJson(ws, {
      type: "error",
      message: "Device name is required.",
    });
    return;
  }

  if (!(device in state.devices)) {
    state.devices[device] = "off";
    state.updatedAt = new Date().toISOString();
  }

  broadcastState("device_added");
}

function handleMessage(ws, rawData) {
  let message;

  try {
    message = JSON.parse(rawData.toString());
  } catch {
    sendJson(ws, {
      type: "error",
      message: "Invalid JSON message.",
    });
    return;
  }

  switch (message.type) {
    case "sensor_update":
      handleSensorUpdate(message);
      break;
    case "device_control":
      handleDeviceControl(message, ws);
      break;
    case "add_device":
      handleAddDevice(message, ws);
      break;
    case "get_status":
      sendJson(ws, {
        type: "state_update",
        reason: "status_request",
        ...snapshot(),
      });
      break;
    case "ping":
      sendJson(ws, { type: "pong", at: new Date().toISOString() });
      break;
    default:
      sendJson(ws, {
        type: "error",
        message: `Unsupported message type: ${message.type}`,
      });
  }
}

app.get("/status", (_req, res) => {
  res.json(snapshot());
});

wss.on("connection", (ws, request) => {
  const role = parseClientRole(request.url);
  ws.isAlive = true;
  ws.role = role;
  clients[role].add(ws);
  updateConnectionCounts();

  sendJson(ws, {
    type: "state_update",
    reason: "connected",
    ...snapshot(),
  });
  broadcastState("client_connected");

  ws.on("pong", () => {
    ws.isAlive = true;
  });

  ws.on("message", (rawData) => handleMessage(ws, rawData));

  ws.on("close", () => {
    clients[role].delete(ws);
    updateConnectionCounts();
    broadcastState("client_disconnected");
  });

  ws.on("error", (error) => {
    console.error(`WebSocket ${role} error:`, error.message);
  });
});

setInterval(() => {
  for (const ws of wss.clients) {
    if (!ws.isAlive) {
      ws.terminate();
      continue;
    }

    ws.isAlive = false;
    ws.ping();
  }
}, 30000);

server.listen(config.port, () => {
  console.log(`HTTP API: http://localhost:${config.port}/status`);
  console.log(`WebSocket: ws://localhost:${config.port}${config.websocketPath}`);
});
