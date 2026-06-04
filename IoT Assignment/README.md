<div align="center">

# 💡 Smart LED Control via MQTT & ESP32

**An IoT publish-subscribe simulation system** demonstrating real-time hardware control.  
Features an ESP32 client simulated in Wokwi subscribing to payloads and a local Python command-line publisher.

[![Python](https://img.shields.io/badge/Python-3.8+-3776AB?style=flat-square&logo=python&logoColor=white)](https://www.python.org)
[![C++](https://img.shields.io/badge/C%2B%2B-Arduino-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)](https://www.arduino.cc/)
[![MQTT](https://img.shields.io/badge/MQTT-HiveMQ-3C3C3D?style=flat-square&logo=mqtt&logoColor=white)](https://mqtt.org)
[![Wokwi](https://img.shields.io/badge/Wokwi-Simulator-2196F3?style=flat-square)](https://wokwi.com)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](./LICENSE)

[Features](#-features) · [Architecture](#-architecture-flow) · [Setup & Execution](#-setup--execution)

</div>

---

## ✨ Features

| Feature | Description |
|---|---|
| 📡 **Real-time MQTT Pub/Sub** | Uses public HiveMQ broker for low-latency transmission. |
| 🐍 **Python Controller** | Keyboard-driven CLI client that publishes control signals. |
| 🎛️ **Simulated Hardware** | ESP32 client with onboard LED reacting instantly to commands. |
| 🔌 **Offline Resiliency** | Automatic reconnection handler for Wi-Fi and MQTT connection drops. |

---

## 🏗️ Architecture Flow

```mermaid
sequenceDiagram
    participant User as Keyboard / User Input
    participant Py as Python Controller
    participant Broker as HiveMQ Broker (Public)
    participant ESP32 as ESP32 Client (Wokwi)
    participant LED as Onboard LED

    User->>Py: Inputs 'y' (ON) / 'n' (OFF)
    Py->>Broker: Publishes payload "ON" / "OFF" to topic
    Broker->>ESP32: Delivers payload to subscriber
    ESP32->>LED: Changes state (HIGH / LOW)
```

---

## 🛠️ Tech Stack

| Layer | Technology | Purpose |
|---|---|---|
| **Firmware** | Arduino C++ / ESP32 WiFi | Controls the board state and subscriptions |
| **Controller** | Python 3 + `paho-mqtt` | Interactive CLI client application |
| **Broker** | HiveMQ (Public Broker) | Routes MQTT messages between publisher and subscriber |
| **Simulation** | Wokwi | Emulates physical ESP32 and WiFi link |

---

## 📂 Project Structure

```
├── sketch.ino               # ESP32 Wi-Fi & MQTT subscriber firmware
└── sample_for_control.py    # Python controller CLI script
```

---

## 🚀 Setup & Execution

### 1. Run the Python Controller

Prerequisites: Python 3 and a virtual environment.

```bash
# Create and activate virtual environment
python3 -m venv venv
source venv/bin/activate

# Install dependencies
pip install paho-mqtt

# Run control script
python sample_for_control.py
```

> [!NOTE]
> **Keyboard CLI Bindings:**
> *   `y` ➔ Turns LED **ON** (publishes `"ON"`)
> *   `n` ➔ Turns LED **OFF** (publishes `"OFF"`)
> *   `q` ➔ **Quit** application

### 2. Run the ESP32 Simulation

1. Open [sketch.ino](file:///home/sudip-kumar-saha/Desktop/CSE-322-Computer%20Network/IoT%20Assignment/sketch.ino).
2. Copy its code and paste it into your [Wokwi](https://wokwi.com/) ESP32 project.
3. Ensure the target topic configured is `buet/cse/2105152/led`.
4. Run the simulator. The ESP32 will connect to `Wokwi-GUEST` WiFi and wait for publish events.
