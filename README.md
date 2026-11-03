# Advanced 2D Football Match Simulator & Tracking System

A realistic, broadcast-grade 2D football match simulator and sports data tracking platform built with **C++17** and **SFML**, inspired by SofaScore and professional tracking suites.

---

## Architecture Overview

The codebase is strictly modular, decoupling mathematical simulation physics (metric pitch units) from hardware rendering (screen pixels), and isolating data collection into distinct pipelines:

```
c:/Users/USER-PC/Desktop/FOOTBALL/
├── CMakeLists.txt              # Unified build definition for Simulator and Test Suite
├── README.md                   # Full documentation & guide
├── run.bat                     # One-click Windows build and run launcher
├── run_linux.sh                # Linux / Fedora KDE build & run script
├── include/
│   ├── Config.hpp              # FIFA pitch dimensions (105m x 68m) & physics constants
│   ├── Entities.hpp            # Player, Ball, TacticalRole, and PlayerState structures
│   ├── Vec2.hpp                # 2D metric vector math (length, normalization, dot product)
│   ├── MatchEngine.hpp         # 60 Hz deterministic AI vs AI simulation loop
│   ├── Tactics.hpp             # Formations (4-3-3, 4-4-2, 4-2-3-1) & dynamic block shifting
│   ├── TrackingSystem.hpp      # High-precision 10 Hz spatial data sampler & CSV serializer
│   ├── EventRecorder.hpp       # Chronological discrete event logger (passes, shots, saves, goals)
│   ├── MatchAnalytics.hpp      # Real-time SofaScore-style match momentum index & possession %
│   ├── SpatialAnalytics.hpp    # 2D pitch discretization (heatmaps, passing networks)
│   └── PitchRenderer.hpp       # SFML graphics pipeline, overlays, and broadcast HUD
├── src/
│   ├── main.cpp                # App entry point, window lifecycle, and input controls
│   ├── game/MatchEngine.cpp    # Physics, passing, shooting, fouls, restarts, and AI decisions
│   ├── ai/Tactics.cpp          # Formation layouts and team compact shape calculations
│   ├── data/EventRecorder.cpp  # Event streaming to data/events.csv
│   ├── analytics/
│   │   ├── MatchAnalytics.cpp  # Territory pressure & exponential moving momentum
│   │   └── SpatialAnalytics.cpp# 21x14 grid histogram accumulation & average positions
│   ├── tracking/TrackingSystem.cpp # 10 Hz tracking capture to data/tracking.csv
│   └── render/PitchRenderer.cpp# FIFA pitch, player kits, HUD, momentum chart, heatmaps
├── tests/
│   └── test_runner.cpp         # Automated unit test suite (determinism, physics, exports)
└── data/                       # Generated sports analytics datasets
    ├── tracking.csv            # 10 Hz player and ball coordinates
    ├── events.csv              # Detailed chronological match events
    ├── players.csv             # End-of-match player workload and attribute metrics
    └── matches.csv             # Final match result, duration, and metadata
```

---

## Interactive Controls

| Input | Action |
| :--- | :--- |
| **`Left Mouse Click`** | Select any player to view their individual spatial heatmap and stats |
| **`[H]`** | Toggle Selected Player Heatmap (ON / OFF) |
| **`[N]`** | Toggle Passing Network Graph (ON / OFF) |
| **`[C]`** | Toggle Ball-Follow Dynamic Camera (ON / OFF) |
| **`[1] / [2] / [3]`** | Home Formations: **4-3-3**, **4-4-2**, **4-2-3-1** |
| **`[7] / [8] / [9]`** | Away Formations: **4-3-3**, **4-4-2**, **4-2-3-1** |
| **`[SPACE]`** | Pause / Resume simulation |
| **`[UP] / [DOWN]`** | Simulation speed ($0.5\times$, $1\times$, $2\times$, $4\times$) |
| **`[E]`** | Flush and export CSV datasets immediately |
| **`[R]`** | Restart match and wipe analytics history |
| **`[ESC]`** | Exit application (auto-flushes all tracking data to disk) |

---

## Building and Running

### On Windows
Double-click `run.bat` or run in PowerShell:
```cmd
.\run.bat
```

### On Linux (Fedora KDE)
```bash
sudo dnf install -y gcc-c++ cmake git SFML-devel dejavu-sans-fonts
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/FootballSim
```

### Running the Automated Test Suite
```bash
./build/FootballSim_Tests
```
