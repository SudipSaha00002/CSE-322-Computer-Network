# 📊 NS-3 Network Simulation: TCP Cubic & CUBIC-FIT

This project evaluates the performance of **TCP Cubic** and **TcpCubicFit** congestion control algorithms under varying network conditions using **ns-3.45**. Simulations are executed across both wired (dumbbell) and wireless (mobility-enabled AP-station) network topologies.

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

## 📂 Key Code Files

*   [cubic-fit-wired.cc](file:///home/sudip-kumar-saha/Desktop/CSE-322-Computer%20Network/NS3-Project/ns-3.45/scratch/cubic-fit-wired.cc): Wired dumbbell point-to-point network helper.
*   [cubic-fit-wireless.cc](file:///home/sudip-kumar-saha/Desktop/CSE-322-Computer%20Network/NS3-Project/ns-3.45/scratch/cubic-fit-wireless.cc): Wireless network with mobility models, propagation loss models, Minstrel HT manager, and radio energy depletion modeling.

---

## 🚀 Execution & Parameters

Commands must be run from the root of the **ns-3.45** directory:

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
> | `--transport_prot` | TCP variant (e.g., `ns3::TcpCubic` or `ns3::TcpCubicFit`) | `ns3::TcpCubic` |
> | `--nWifi` / `--nFlows` | Number of client nodes / active TCP flows | 6 / 2 |
> | `--errorRate` | Packet drop probability | `0.001` |
> | `--isMobile` | Toggles RandomWalk2d node mobility | `true` |
> | `--nodeSpeed` | Speed in m/s (only if `isMobile=true`) | `2.0` |

---

## 📈 Performance Metrics Analyzed

At the end of each simulation, the following metrics are output to console and appended to a results output file:
1. **Total Throughput (Mbps)**
2. **Average End-to-End Delay (ns)**
3. **Packet Delivery Ratio (PDR %)**
4. **Packet Drop Ratio (%)**
5. **Jain's Fairness Index**
6. **Total Energy Consumed (Joules)** (Wireless only)
