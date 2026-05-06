import React, { useEffect, useMemo, useRef, useState } from "react";
import {
  AirVent,
  BellRing,
  CircuitBoard,
  Droplets,
  Fan,
  Flame,
  Home,
  Lightbulb,
  Lock,
  Plus,
  Power,
  ShieldCheck,
  Snowflake,
  Sprout,
  Sun,
  Thermometer,
  Wifi,
  WifiOff,
  Wind,
  Zap,
} from "lucide-react";
import { API_BASE_URL, WS_URL } from "./config";

const DEFAULT_STATE = {
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
    fire: null,
    moisture: null,
  },
  connected: {
    espClients: 0,
    dashboardClients: 0,
  },
  updatedAt: null,
};

const SENSOR_META = {
  temperature: {
    label: "Temperature",
    unit: "C",
    icon: Thermometer,
    max: 50,
    color: "#ef4444",
    bg: "from-rose-50 to-orange-50",
  },
  humidity: {
    label: "Humidity",
    unit: "%",
    icon: Droplets,
    max: 100,
    color: "#0ea5e9",
    bg: "from-sky-50 to-cyan-50",
  },
  gas: {
    label: "Gas",
    unit: "ppm",
    icon: Wind,
    max: 1200,
    color: "#f59e0b",
    bg: "from-amber-50 to-yellow-50",
  },
  fire: {
    label: "Fire",
    unit: "raw",
    icon: Flame,
    max: 4095,
    color: "#dc2626",
    bg: "from-red-50 to-zinc-50",
  },
};

const DEVICE_META = {
  light: { label: "Light", icon: Lightbulb },
  fan: { label: "Fan", icon: Fan },
  door: { label: "Door", icon: Lock },
  ac: { label: "AC", icon: Snowflake },
};

function normalizeNumber(value) {
  return typeof value === "number" && Number.isFinite(value) ? value : 0;
}

function displayValue(value, fallback = "--") {
  return typeof value === "number" && Number.isFinite(value) ? value : fallback;
}

function useSmartHomeSocket() {
  const [homeState, setHomeState] = useState(DEFAULT_STATE);
  const [isConnected, setIsConnected] = useState(false);
  const [sensorHistory, setSensorHistory] = useState({
    temperature: [],
    humidity: [],
    gas: [],
    fire: [],
  });
  const socketRef = useRef(null);
  const reconnectTimerRef = useRef(null);

  useEffect(() => {
    let closedByEffect = false;

    async function loadStatus() {
      try {
        const response = await fetch(`${API_BASE_URL}/status`);
        const status = await response.json();
        setHomeState((current) => ({ ...current, ...status }));
        if (status.sensors) {
          appendSensorHistory(status.sensors);
        }
      } catch {
        // WebSocket reconnect below keeps the dashboard ready while backend starts.
      }
    }

    function appendSensorHistory(sensors) {
      setSensorHistory((current) => {
        const next = { ...current };

        for (const sensorName of Object.keys(SENSOR_META)) {
          const value = sensors[sensorName];
          if (typeof value === "number" && Number.isFinite(value)) {
            next[sensorName] = [
              ...current[sensorName],
              { value, time: Date.now() },
            ].slice(-28);
          }
        }

        return next;
      });
    }

    function connect() {
      const socket = new WebSocket(WS_URL);
      socketRef.current = socket;

      socket.addEventListener("open", () => {
        setIsConnected(true);
        socket.send(JSON.stringify({ type: "get_status" }));
      });

      socket.addEventListener("message", (event) => {
        let message;

        try {
          message = JSON.parse(event.data);
        } catch {
          return;
        }

        if (message.type === "state_update") {
          setHomeState((current) => ({ ...current, ...message }));
        }

        if (message.type === "sensor_update" || message.type === "state_update") {
          const sensors = message.sensors;
          if (sensors) {
            setHomeState((current) => ({ ...current, sensors }));
            appendSensorHistory(sensors);
          }
        }
      });

      socket.addEventListener("close", () => {
        setIsConnected(false);
        if (!closedByEffect) {
          reconnectTimerRef.current = window.setTimeout(connect, 2000);
        }
      });

      socket.addEventListener("error", () => {
        socket.close();
      });
    }

    loadStatus();
    connect();

    return () => {
      closedByEffect = true;
      window.clearTimeout(reconnectTimerRef.current);
      socketRef.current?.close();
    };
  }, []);

  function sendMessage(message) {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(JSON.stringify(message));
    }
  }

  return { homeState, isConnected, sensorHistory, sendMessage };
}

function buildLinePath(points, width, height, max) {
  const safePoints = points.length > 1 ? points : [{ value: 0 }, { value: 0 }];

  return safePoints
    .map((point, index) => {
      const x = (index / (safePoints.length - 1)) * width;
      const y = height - (normalizeNumber(point.value) / max) * height;
      return `${index === 0 ? "M" : "L"} ${x.toFixed(1)} ${Math.max(4, y).toFixed(1)}`;
    })
    .join(" ");
}

function SensorMiniCard({ name, value, points }) {
  const meta = SENSOR_META[name];
  const Icon = meta.icon;
  const numericValue = normalizeNumber(value);
  const percent = Math.min(100, Math.round((numericValue / meta.max) * 100));
  const path = buildLinePath(points, 150, 54, Math.max(meta.max, numericValue, 1));

  return (
    <article className={`rounded-2xl border border-white/70 bg-gradient-to-br ${meta.bg} p-4 shadow-soft`}>
      <div className="flex items-start justify-between gap-3">
        <div className="flex min-w-0 items-center gap-3">
          <span className="grid h-11 w-11 place-items-center rounded-xl bg-white/85 shadow-sm" style={{ color: meta.color }}>
            <Icon size={22} />
          </span>
          <div className="min-w-0">
            <p className="truncate text-sm font-semibold text-slate-500">{meta.label}</p>
            <p className="text-2xl font-black text-slate-950">
              {displayValue(value)} <span className="text-sm font-bold text-slate-500">{meta.unit}</span>
            </p>
          </div>
        </div>
        <span className="rounded-full bg-white/80 px-2.5 py-1 text-xs font-black text-slate-500">{percent}%</span>
      </div>

      <div className="mt-4 h-16 rounded-xl bg-white/55 p-2">
        <svg viewBox="0 0 150 54" className="h-full w-full" role="img" aria-label={`${meta.label} trend`}>
          <defs>
            <linearGradient id={`fill-${name}`} x1="0" x2="0" y1="0" y2="1">
              <stop offset="0%" stopColor={meta.color} stopOpacity="0.28" />
              <stop offset="100%" stopColor={meta.color} stopOpacity="0.02" />
            </linearGradient>
          </defs>
          <path d={`${path} L 150 54 L 0 54 Z`} fill={`url(#fill-${name})`} />
          <path d={path} fill="none" stroke={meta.color} strokeLinecap="round" strokeWidth="4" />
        </svg>
      </div>
    </article>
  );
}

function TemperatureFocus({ sensors, points }) {
  const value = displayValue(sensors.temperature, 0);
  const humidity = displayValue(sensors.humidity, 0);
  const path = buildLinePath(points, 360, 128, Math.max(50, normalizeNumber(sensors.temperature), 1));

  return (
    <article className="rounded-3xl border border-white/70 bg-white/80 p-5 shadow-soft backdrop-blur">
      <div className="flex items-center justify-between gap-4">
        <div>
          <p className="text-sm font-bold uppercase tracking-[0.18em] text-slate-400">Climate Curve</p>
          <h2 className="mt-1 text-3xl font-black text-slate-950">{value} C</h2>
        </div>
        <div className="grid h-14 w-14 place-items-center rounded-2xl bg-rose-100 text-rose-600">
          <Thermometer size={28} />
        </div>
      </div>
      <div className="mt-5 h-40 rounded-2xl bg-slate-950 p-4">
        <svg viewBox="0 0 360 128" className="h-full w-full" role="img" aria-label="Temperature chart">
          <defs>
            <linearGradient id="temperature-fill" x1="0" x2="0" y1="0" y2="1">
              <stop offset="0%" stopColor="#f97316" stopOpacity="0.52" />
              <stop offset="100%" stopColor="#f97316" stopOpacity="0" />
            </linearGradient>
          </defs>
          {[24, 56, 88, 120].map((line) => (
            <line key={line} x1="0" x2="360" y1={line} y2={line} stroke="#334155" strokeDasharray="6 8" />
          ))}
          <path d={`${path} L 360 128 L 0 128 Z`} fill="url(#temperature-fill)" />
          <path d={path} fill="none" stroke="#fb923c" strokeLinecap="round" strokeWidth="5" />
          <circle cx="350" cy="24" r="5" fill="#fed7aa" />
        </svg>
      </div>
      <div className="mt-4 grid grid-cols-2 gap-3">
        <div className="rounded-2xl bg-slate-100 p-3">
          <p className="text-xs font-bold text-slate-500">Humidity</p>
          <p className="text-xl font-black text-slate-950">{humidity}%</p>
        </div>
        <div className="rounded-2xl bg-slate-100 p-3">
          <p className="text-xs font-bold text-slate-500">Air Quality</p>
          <p className="text-xl font-black text-slate-950">{displayValue(sensors.gas, 0)}</p>
        </div>
      </div>
    </article>
  );
}

function SuggestionBox({ sensors }) {
  const suggestions = useMemo(() => {
    const output = [];
    const temperature = normalizeNumber(sensors.temperature);
    const humidity = normalizeNumber(sensors.humidity);
    const gas = normalizeNumber(sensors.gas);
    const fire = normalizeNumber(sensors.fire);

    if (temperature >= 32) {
      output.push({ icon: AirVent, title: "Cool the room", text: "Temperature is high. Turn on fan or AC.", tone: "rose" });
    } else {
      output.push({ icon: ShieldCheck, title: "Climate stable", text: "Temperature is within the comfort range.", tone: "emerald" });
    }

    if (gas >= 500) {
      output.push({ icon: BellRing, title: "Ventilate now", text: "Gas reading is rising. Open ventilation and inspect source.", tone: "amber" });
    }

    if (fire >= 1800) {
      output.push({ icon: Flame, title: "Fire risk alert", text: "Fire sensor is above the safe baseline.", tone: "rose" });
    }

    if (humidity > 0 && humidity < 35) {
      output.push({ icon: Droplets, title: "Dry air detected", text: "Humidity is low. Consider humidification.", tone: "sky" });
    }

    return output.slice(0, 4);
  }, [sensors]);

  const toneClass = {
    emerald: "border-emerald-200 bg-emerald-50 text-emerald-800",
    amber: "border-amber-200 bg-amber-50 text-amber-900",
    rose: "border-rose-200 bg-rose-50 text-rose-900",
    sky: "border-sky-200 bg-sky-50 text-sky-900",
  };

  return (
    <section className="rounded-3xl border border-white/70 bg-white/85 p-5 shadow-soft backdrop-blur">
      <div className="mb-4 flex items-center gap-3">
        <span className="grid h-11 w-11 place-items-center rounded-2xl bg-slate-950 text-white">
          <Zap size={21} />
        </span>
        <div>
          <h2 className="text-lg font-black text-slate-950">Environment Suggestions</h2>
          <p className="text-sm font-medium text-slate-500">Automation hints from live sensor values</p>
        </div>
      </div>
      <div className="grid gap-3">
        {suggestions.map((item) => {
          const Icon = item.icon;
          return (
            <article className={`rounded-2xl border p-4 ${toneClass[item.tone]}`} key={item.title}>
              <div className="flex items-start gap-3">
                <Icon className="mt-0.5 shrink-0" size={21} />
                <div>
                  <h3 className="font-black">{item.title}</h3>
                  <p className="mt-1 text-sm font-medium opacity-80">{item.text}</p>
                </div>
              </div>
            </article>
          );
        })}
      </div>
    </section>
  );
}

function HomeTexture({ sensors, isConnected }) {
  return (
    <section className="home-texture relative overflow-hidden rounded-3xl border border-white/70 p-6 shadow-soft">
      <div className="relative z-10 flex items-center justify-between gap-4">
        <div>
          <p className="text-sm font-black uppercase tracking-[0.18em] text-white/70">Live Home</p>
          <h2 className="mt-2 max-w-sm text-4xl font-black leading-tight text-white">Comfort map and room automation</h2>
        </div>
        <span className={`flex items-center gap-2 rounded-full px-4 py-2 text-sm font-black ${isConnected ? "bg-emerald-400 text-emerald-950" : "bg-white/20 text-white"}`}>
          {isConnected ? <Wifi size={17} /> : <WifiOff size={17} />}
          {isConnected ? "Live" : "Offline"}
        </span>
      </div>

      <div className="relative z-10 mt-10 grid grid-cols-[1fr_0.8fr] gap-4">
        <div className="house-card rounded-3xl bg-white/18 p-5 text-white ring-1 ring-white/30 backdrop-blur">
          <div className="mx-auto grid h-44 max-w-xs place-items-center rounded-[2rem] border border-white/25 bg-white/15">
            <Home size={92} strokeWidth={1.5} />
          </div>
          <div className="mt-5 grid grid-cols-3 gap-3 text-center">
            <div className="rounded-2xl bg-white/18 p-3">
              <Thermometer className="mx-auto" size={20} />
              <p className="mt-1 text-lg font-black">{displayValue(sensors.temperature)} C</p>
            </div>
            <div className="rounded-2xl bg-white/18 p-3">
              <Droplets className="mx-auto" size={20} />
              <p className="mt-1 text-lg font-black">{displayValue(sensors.humidity)}%</p>
            </div>
            <div className="rounded-2xl bg-white/18 p-3">
              <Flame className="mx-auto" size={20} />
              <p className="mt-1 text-lg font-black">{displayValue(sensors.fire, 0)}</p>
            </div>
          </div>
        </div>
        <div className="grid content-between gap-4">
          <div className="rounded-3xl bg-white/90 p-4">
            <Sun className="text-amber-500" size={24} />
            <p className="mt-4 text-sm font-bold text-slate-500">Room mood</p>
            <p className="text-2xl font-black text-slate-950">Balanced</p>
          </div>
          <div className="rounded-3xl bg-slate-950/70 p-4 text-white ring-1 ring-white/20">
            <CircuitBoard className="text-cyan-300" size={24} />
            <p className="mt-4 text-sm font-bold text-white/60">ESP clients</p>
            <p className="text-2xl font-black">Connected</p>
          </div>
        </div>
      </div>
    </section>
  );
}

function LightControl({ devices, sendMessage }) {
  const isOn = devices.light === "on";

  function toggleLight() {
    sendMessage({
      type: "device_control",
      device: "light",
      state: isOn ? "off" : "on",
    });
  }

  return (
    <section className={`rounded-3xl border p-5 shadow-soft transition ${isOn ? "border-yellow-200 bg-yellow-50" : "border-white/70 bg-white/80"}`}>
      <div className="flex items-center justify-between">
        <div>
          <p className="text-sm font-bold text-slate-500">Light Configuration</p>
          <h2 className="mt-1 text-2xl font-black text-slate-950">Room Light</h2>
        </div>
        <button
          className={`relative h-9 w-16 rounded-full p-1 transition ${isOn ? "bg-yellow-400" : "bg-slate-200"}`}
          onClick={toggleLight}
          type="button"
          aria-label="Toggle room light"
        >
          <span className={`block h-7 w-7 rounded-full bg-white shadow transition ${isOn ? "translate-x-7" : "translate-x-0"}`} />
        </button>
      </div>

      <button
        className={`mt-6 grid h-52 w-full place-items-center rounded-3xl border transition ${isOn ? "border-yellow-300 bg-yellow-100 text-yellow-500 shadow-glow" : "border-slate-200 bg-white/50 text-slate-300"}`}
        onClick={toggleLight}
        type="button"
      >
        <Lightbulb size={104} strokeWidth={1.35} fill={isOn ? "#fde047" : "transparent"} />
      </button>

      <div className="mt-4 flex items-center justify-between rounded-2xl bg-white/75 px-4 py-3">
        <span className="text-sm font-bold text-slate-500">Current state</span>
        <span className={`rounded-full px-3 py-1 text-sm font-black ${isOn ? "bg-yellow-300 text-yellow-950" : "bg-slate-200 text-slate-600"}`}>
          {isOn ? "ON" : "OFF"}
        </span>
      </div>
    </section>
  );
}

function DeviceRail({ devices, sendMessage }) {
  const [newDevice, setNewDevice] = useState("");

  function toggleDevice(device, currentState) {
    sendMessage({
      type: "device_control",
      device,
      state: currentState === "on" ? "off" : "on",
    });
  }

  function addDevice(event) {
    event.preventDefault();
    const device = newDevice.trim();
    if (!device) {
      return;
    }

    sendMessage({ type: "add_device", device });
    setNewDevice("");
  }

  return (
    <section className="rounded-3xl border border-white/70 bg-white/80 p-5 shadow-soft backdrop-blur">
      <div className="mb-4 flex items-center justify-between">
        <h2 className="text-lg font-black text-slate-950">Other Devices</h2>
        <Power size={20} className="text-slate-400" />
      </div>
      <div className="grid grid-cols-2 gap-3">
        {Object.entries(devices)
          .filter(([device]) => device !== "light")
          .map(([device, state]) => {
            const meta = DEVICE_META[device] || { label: device.replaceAll("_", " "), icon: Power };
            const Icon = meta.icon;
            const isOn = state === "on";

            return (
              <button
                className="rounded-2xl border border-slate-200 bg-white p-4 text-left shadow-sm transition hover:-translate-y-0.5 hover:border-slate-300"
                key={device}
                onClick={() => toggleDevice(device, state)}
                type="button"
              >
                <div className="flex items-center justify-between">
                  <span className="grid h-11 w-11 place-items-center rounded-xl bg-slate-100 text-slate-700">
                    <Icon size={22} />
                  </span>
                  <span className={`h-3 w-3 rounded-full ${isOn ? "bg-emerald-500" : "bg-amber-800"}`} />
                </div>
                <p className="mt-4 truncate text-sm font-black capitalize text-slate-950">{meta.label}</p>
                <p className="text-xs font-bold text-slate-500">{isOn ? "Running" : "Stopped"}</p>
              </button>
            );
          })}
      </div>

      <form className="mt-4 grid grid-cols-[1fr_46px] gap-2" onSubmit={addDevice}>
        <input
          className="min-w-0 rounded-2xl border border-slate-200 bg-white px-4 text-sm font-semibold text-slate-800 outline-none transition focus:border-slate-400"
          aria-label="New device name"
          value={newDevice}
          onChange={(event) => setNewDevice(event.target.value)}
          placeholder="Add device"
        />
        <button className="grid h-12 place-items-center rounded-2xl bg-slate-950 text-white" type="submit" aria-label="Add device">
          <Plus size={19} />
        </button>
      </form>
    </section>
  );
}

export default function App() {
  const { homeState, isConnected, sensorHistory, sendMessage } = useSmartHomeSocket();

  return (
    <main className="min-h-screen bg-[#eef3f8] bg-texture px-4 py-5 text-slate-950 sm:px-6 lg:px-8">
      <header className="mx-auto mb-5 flex max-w-[1500px] flex-col gap-4 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <p className="text-sm font-black uppercase tracking-[0.18em] text-slate-400">ESP32 Automation</p>
          <h1 className="mt-1 text-4xl font-black leading-tight text-slate-950 sm:text-5xl">Smart Home Dashboard</h1>
        </div>
        <div className={`flex w-fit items-center gap-2 rounded-full border px-4 py-2 text-sm font-black shadow-sm ${isConnected ? "border-emerald-200 bg-emerald-50 text-emerald-700" : "border-rose-200 bg-rose-50 text-rose-700"}`}>
          {isConnected ? <Wifi size={18} /> : <WifiOff size={18} />}
          {isConnected ? "Realtime Connected" : "Reconnecting"}
        </div>
      </header>

      <section className="mx-auto grid max-w-[1500px] gap-5 xl:grid-cols-[320px_minmax(420px,1fr)_360px]">
        <aside className="grid gap-4">
          <TemperatureFocus sensors={homeState.sensors} points={sensorHistory.temperature} />
          <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-1">
            {Object.keys(SENSOR_META).map((name) => (
              <SensorMiniCard
                key={name}
                name={name}
                value={homeState.sensors[name]}
                points={sensorHistory[name] || []}
              />
            ))}
          </div>
        </aside>

        <section className="grid content-start gap-5">
          <HomeTexture sensors={homeState.sensors} isConnected={isConnected} />
          <SuggestionBox sensors={homeState.sensors} />
        </section>

        <aside className="grid content-start gap-5">
          <LightControl devices={homeState.devices} sendMessage={sendMessage} />
          <DeviceRail devices={homeState.devices} sendMessage={sendMessage} />
          <section className="grid grid-cols-2 gap-3">
            <div className="rounded-3xl border border-white/70 bg-white/80 p-4 shadow-soft">
              <p className="text-sm font-bold text-slate-500">ESP Clients</p>
              <p className="mt-2 text-3xl font-black text-slate-950">{homeState.connected?.espClients ?? 0}</p>
            </div>
            <div className="rounded-3xl border border-white/70 bg-white/80 p-4 shadow-soft">
              <p className="text-sm font-bold text-slate-500">Dashboards</p>
              <p className="mt-2 text-3xl font-black text-slate-950">{homeState.connected?.dashboardClients ?? 0}</p>
            </div>
          </section>
        </aside>
      </section>
    </main>
  );
}
