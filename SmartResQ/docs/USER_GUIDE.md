# SmartResQ – User Guide & Demonstration Manual

> **Project:** SmartResQ – Emergency Ambulance Routing and Shortest Path Optimization  
> **Target Audience:** Evaluators, Professors, Students, and Demonstrators  
> **URL:** `http://127.0.0.1:5000`  

---

## 1. System Requirements & Setup

### 1.1 Prerequisites
- **Operating System:** Windows 10/11, Ubuntu 20.04+, or macOS.
- **C++ Toolchain:** GCC 9+ (MinGW-w64 on Windows), Clang 10+, or MSVC 2019/2022 supporting ISO C++17.
- **Build System:** CMake ($\ge 3.16$) and Ninja (or Make).
- **Python:** Python 3.10 or higher with pip.

### 1.2 Installing Python Dependencies
From a terminal:
```bash
pip install Flask flask-cors
```

---

## 2. Compilation & Execution Commands

### Step 1: Compile Native C++ DSA Engine
From the project root:
```bash
cd SmartResQ/cpp_engine

# Configure build with Ninja (or use default generator)
cmake -G Ninja -B build -S .

# Build both the engine and unit tests
cmake --build build

# Verify C++ unit tests (20/20 must pass)
./build/smartresq_tests.exe
```

### Step 2: Run Automated Master Test Battery
Verify the complete end-to-end Python Flask and C++ engine bridge:
```bash
cd SmartResQ/backend
python verify_all.py
```
*(All 10 test areas will display `[PASS]`)*

### Step 3: Launch the Flask Backend Service
```bash
cd SmartResQ/backend
python app.py
```
Expected terminal output:
```
================================================================
 SmartResQ Flask Backend Active on http://127.0.0.1:5000
 C++ Engine Binary : E:\DSA_Project\SmartResQ\cpp_engine\build\smartresq_engine.exe
 C++ Binary Status : EXISTS
================================================================
 * Running on http://127.0.0.1:5000
```

### Step 4: Open the Web Dashboard
Open your web browser and navigate to:
```
http://127.0.0.1:5000
```

---

## 3. Step-by-Step Demonstration Sequence (10 Steps)

Follow this 10-step sequence for a complete live demonstration:

```
[1. Open Dashboard]  ──>  [2. Display City Graph]  ──>  [3. Select Incident Node]
                                                                  │
                                                                  ▼
[6. Find Hospital]   <──  [5. Calculate Route]    <──  [4. Find Ambulance]
        │
        ▼
[7. Block a Road]    ──>  [8. Recalculate Route]  ──>  [9. Compare BFS/Dijkstra]
                                                                  │
                                                                  ▼
                                                      [10. Reset Simulation]
```

### Step 1: Open Dashboard
- Open `http://127.0.0.1:5000`.
- Verify the header displays:
  - SmartResQ branding with `DSA C++17` badge.
  - Green pulse indicator: **C++ DSA Engine Online**.
  - Simulation counters: `0 Blocked Roads`, `0 Dispatched Units`.

### Step 2: Display City Graph
- Observe the Leaflet dark navy map:
  - **18 City Junctions:** Slate circular nodes with coordinate labels.
  - **3 Ambulance Stations:** Electric blue markers (`L5`, `L10`, `L16`).
  - **3 Trauma Hospitals:** Emerald green markers (`L2`, `L8`, `L14`).
  - **29 Bidirectional Roads:** Dotted lines connecting the nodes. Hover over any road to inspect its ID and distance in kilometers.

### Step 3: Select Emergency Location
- Under the **Emergency Mission** tab:
  - Open the *Incident Location* dropdown and select **`Central Junction (L1)`**  
    *(Alternatively: click directly on node `L1` on the map).*
  - The node `L1` will turn red with a pulsing highlight.
  - The `Calculate & Dispatch Nearest Ambulance` button becomes enabled.

### Step 4: Find Nearest Ambulance
- Click **`⚡ Calculate & Dispatch Nearest Ambulance`**.
- The C++ engine scans available units via Dijkstra:
  - `AMB-01` at `L5`: distance $3.50\text{ km}$
  - `AMB-02` at `L10`: distance $4.70\text{ km}$
  - `AMB-03` at `L16`: distance $1.10\text{ km}$
- **Result:** **`Ambulance Charlie (AMB-03)`** is dispatched.
- The unit is locked in state to prevent double dispatch. Dispatched counter increments to `1 Dispatched Unit`.

### Step 5: Calculate Shortest Route
- Inspect the map and route cards:
  - **Leg 1 (Ambulance Response):** Solid amber polyline drawn from `L16` to `L1` ($1.10\text{ km}$).
  - Waypoints flow displays: `L16 ➔ L1`.

### Step 6: Find Nearest Hospital
- The system simultaneously identifies the closest medical center for patient transfer:
  - **Selected Primary:** `City General Hospital (HOSP-01)` at `L2` ($1.20\text{ km}$).
  - **Leg 2 (Hospital Transport):** Cyan dashed polyline drawn from `L1` to `L2` ($1.20\text{ km}$).
  - **Total Mission Distance:** $1.10\text{ km} + 1.20\text{ km} = \mathbf{2.30\text{ km}}$.
  - **Ranked Alternatives:**
    - `Apex Trauma Center (L8)`: $4.60\text{ km}$
    - `Sunrise Specialty Hospital (L14)`: $7.00\text{ km}$

### Step 7: Block a Road
- Switch to the **🚧 Road Blockages** tab in the sidebar.
- Select road **`Road R01: L1 ↔ L2 (1.2 km)`** in the dropdown.
- Click **`🛑 Block Road`**  
  *(Alternatively: click directly on the road segment between L1 and L2 on the map).*
- Road `R01` changes immediately to a **bold dashed red line** marked `🛑 BLOCKED`.
- Active road closures counter updates to `1 Blocked Road`.

### Step 8: Recalculate Route
- Under the **Detour Recalculation** section:
  - Set Origin to `L1` and Destination to `L2`.
  - Click **`⚡ Compute Detour Route`**.
  - The C++ Dijkstra solver excludes `R01` and detours via Cyber City:
    $$\text{Path: } L1 \to L17 \to L2 \quad (\text{Roads } R04, R06)$$
  - Detour Distance: **$3.10\text{ km}$** (an extra $+1.90\text{ km}$ detour).
  - Magenta detour route renders on the map.

### Step 9: Compare BFS and Dijkstra
- Switch to the **⚖️ BFS vs Dijkstra** tab in the sidebar.
- Set Origin to `L1 (Central Junction)` and Destination to `L8 (Apex Trauma Center)`.
- Click **`⚡ Run Comparative Analysis`**.
- Inspect side-by-side cards:
  - **Dijkstra Card:** Distance: $4.60\text{ km}$, Hops: 3, Min-Heap PQ ($\mathcal{O}((V+E)\log V)$), Path: `L1 ➔ L16 ➔ L9 ➔ L8`.
  - **BFS Card:** Distance: $4.60\text{ km}$, Hops: 3, FIFO Queue ($\mathcal{O}(V+E)$), Path: `L1 ➔ L16 ➔ L9 ➔ L8`.
- Click **`Show Dijkstra on Map`** to highlight Dijkstra in amber.
- Click **`Show BFS on Map`** to highlight BFS in purple.

### Step 10: Reset Simulation
- Click the **`🔄 Reset Simulation`** button in the top right header.
- The C++ engine clears all blockages and releases all ambulances.
- Road `R01` turns back to normal blue-gray.
- All result panels clear, and the counters return to `0 Blocked Roads` and `0 Dispatched Units`.

---

## 4. UI Dashboard Architecture & Visual Elements

```
+-----------------------------------------------------------------------------------------------+
| 🚑 SmartResQ [DSA C++17]     🛑 0 Blocked Roads   🚑 0 Dispatched   🟢 C++ Online  [🔄 Reset] |
+------------------------------------+----------------------------------------------------------+
| SIDEBAR CONTROL DECK               | INTERACTIVE LEAFLET CITY MAP                             |
|                                    |                                                          |
| [🚨 Mission] [🚧 Block] [⚖️ Compare]|  [🎯 Recenter]  [🧹 Clear Routes]                        |
| ---------------------------------- |                                                          |
| 🚨 EMERGENCY INCIDENT              |                       (L15)                              |
| Select: [ Central Junction (L1)  ] |             (L4)------------------(L6)                   |
| [⚡ Dispatch Nearest Ambulance   ] |              |                     |                     |
|                                    |         (L2)医院                  |                     |
| 🚑 DISPATCHED AMBULANCE            |          /   \                   (L17)                  |
| Unit: Ambulance Charlie (AMB-03)   |       [R01]   \                  /                      |
| Travel Distance: 1.10 km           |        /       \                /                       |
|                                    |    (L1)🚨       (L3)           (L7)                     |
| 🏥 NEAREST HOSPITAL                |      |    \      |              |                       |
| Hospital: City General (HOSP-01)   |      |     \     |             (L8)医院                 |
| Transfer Distance: 1.20 km         |    (L16)🚑  \   (L5)🚑                                  |
|                                    |               \                                          |
| 🗺️ MISSION SUMMARY                 |               (L9)-------(L10)🚑                         |
| Leg 1: 1.10 km | Leg 2: 1.20 km    |                                                          |
| Total: 2.30 km                     |                                                          |
| Waypoints: L16 ➔ L1 ➔ L2           | +------------------------------------------------------+ |
|                                    | | LEGEND:  ● Junction  🚑 Ambulance  🏥 Hospital  🚨 Incident| |
|                                    | +------------------------------------------------------+ |
+------------------------------------+----------------------------------------------------------+
```

---

## 5. Troubleshooting Guide

| Issue | Root Cause | Solution |
|---|---|---|
| **C++ Engine Not Found** | Binary has not been compiled yet | Run `cmake --build cpp_engine/build` from the project root. |
| **Port 5000 in Use** | Another service is using port 5000 | Set port via environment variable: `PORT=5050 python app.py` or terminate the conflicting process. |
| **ModuleNotFoundError: flask** | Virtual environment missing packages | Run `pip install Flask flask-cors`. |
| **C++ Compiler Not Recognized** | MinGW / MSVC not in PATH | Verify compiler installation via `g++ --version` or launch from the appropriate developer prompt. |
| **Tiles Fail to Load** | Offline environment | The graph vertices, edges, tooltips, and routes will render correctly even if external map tiles cannot reach the CDN. |

