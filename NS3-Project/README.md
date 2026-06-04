<div align="center">

# 📊 NS-3 Network Simulation: TCP Cubic & CUBIC-FIT

**Performance evaluation of congestion control variants** under diverse configurations.  
Runs wired dumbbell point-to-point and wireless mobile AP-station simulations using **ns-3.45**.

[![ns-3](https://img.shields.io/badge/ns--3-3.45-FF6600?style=flat-square)](https://www.nsnam.org)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)](https://isocpp.org)
[![Python](https://img.shields.io/badge/Python-3.8+-3776AB?style=flat-square&logo=python&logoColor=white)](https://www.python.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](./LICENSE)

[Features](#-features) · [Topologies](#-topologies) · [Execution & Parameters](#-execution--parameters) · [Metrics](#-performance-metrics-analyzed)

</div>

---

## ✨ Features

| Feature | Description |
|---|---|
| 🎛️ **Algorithmic Evaluation** | Side-by-side comparison of `ns3::TcpCubic` and custom `ns3::TcpCubicFit`. |
| 🔀 **Topology Models** | Supports both Wired Dumbbell Point-to-Point and Wireless AP-STA simulation setups. |
| 🏃‍♂️ **Node Mobility** | Simulates mobile stations in 2D space using `ns3::RandomWalk2dMobilityModel`. |
| 🔋 **Energy Monitoring** | Integrates `ns3::BasicEnergySource` to measure Wi-Fi interface power consumption (Joules). |
| 📈 **Automation Scripts** | Shell wrappers to batch run simulation scenarios and generate data files. |

---

## 🏗️ Topologies

### 1. Wired Dumbbell Layout
`cubic-fit-wired.cc` simulates multiple TCP flows crossing a single bottleneck link:
```
  Left Leaves (S_n)                             Right Leaves (D_n)
       n2  ───┐                                     ┌───  n6
       n3  ───┼──> Router 0 ═════════════> Router 1 ┼───> n7
       n4  ───┘       [ Bottleneck Link: 10Mbps ]   └───  n8
```

### 2. Wireless AP-Station Layout
`cubic-fit-wireless.cc` simulates mobile/static stations communicating via center-positioned APs connected by a Point-to-Point backbone:
```
   [Wifi Left: 192.168.20.0]                  [Wifi Right: 192.168.30.0]
     STAs * * * * AP (n0) ────────────── AP (n1) * * * * STAs
                          [P2P: 50Mbps]
```

---

## 🛠️ Tech Stack

| Layer | Technology | Purpose |
|---|---|---|
| **Core Engine** | ns-3.45 (C++17) | Discrete-event network simulation framework |
| **Routing & Mac** | Minstrel HT, Range Propagation | Manages Wi-Fi physical channels and dynamic rate adaptation |
| **Analysis** | FlowMonitor | Tracks packet statistics, latencies, and drops |
| **Plotting** | Python 3 + Matplotlib | Compiles results into comparative performance graphs |

---

## 📂 Project Structure

```
├── ns-3.45/
│   ├── scratch/
│   │   ├── cubic-fit-wired.cc       # Wired dumbbell topology source
│   │   └── cubic-fit-wireless.cc    # Wireless AP-STA mobility and energy source
│   └── src/internet/model/
│       ├── tcp-cubic-fit.cc         # CUBIC-FIT congestion logic implementation
│       └── tcp-cubic-fit.h          # CUBIC-FIT module headers
```

---

## 🚀 Execution & Parameters

All scripts should be executed from the root of the **ns-3.45** directory:

### Run Wired Simulation
```bash
./ns3 run "scratch/cubic-fit-wired --bw=10Mbps --p2pDelay=100ms --errorRate=0.01 --transport_prot=ns3::TcpCubicFit --nWifi=10 --nFlows=10"
```

### Run Wireless Simulation
```bash
./ns3 run "scratch/cubic-fit-wireless --nFlows=2 --nWifi=6 --errorRate=0.001 --transport_prot=ns3::TcpCubicFit --nodeSpeed=2.0 --isMobile=true"
```

> [!TIP]
> **Key Configuration Parameters:**
> | Argument | Description | Default |
> |---|---|---|
> | `--transport_prot` | TCP variant (e.g. `ns3::TcpCubic` or `ns3::TcpCubicFit`) | `ns3::TcpCubic` |
> | `--nWifi` / `--nFlows` | Number of client nodes / active TCP flows | 6 / 2 |
> | `--errorRate` | Packet drop probability | `0.001` |
> | `--isMobile` | Toggles RandomWalk2d node mobility | `true` |
> | `--nodeSpeed` | Speed in m/s (only if `isMobile=true`) | `2.0` |

---

## 📈 Performance Metrics Analyzed

At the end of each simulation, results are printed to the console and appended to your data files:
1. **Total Throughput (Mbps)** — Rate of successful packet deliveries.
2. **Average End-to-End Delay (ns)** — Average traversal latency.
3. **Packet Delivery Ratio (PDR %)** — Percentage of sent packets successfully received.
4. **Packet Drop Ratio (%)** — Percentage of packets lost in congestion/channel interference.
5. **Jain's Fairness Index** — Quantitative measure of throughput equity among competing flows.
6. **Total Energy Consumed (Joules)** — Power consumed by wireless radios (Tx/Rx/Idle).
