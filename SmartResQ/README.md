# SmartResQ – Emergency Ambulance Routing and Shortest Path Optimization

> **Third-Year Computer Engineering Mini-Project**  
> **Course:** Data Structures and Algorithms (DSA)  
> **Author:** SmartResQ Team  
> **Core Algorithms:** Graph Modeling (Adjacency List) & Dijkstra's Algorithm (Min-Heap Priority Queue)

---

## 📌 Project Objective

**SmartResQ** is an educational simulation that optimizes emergency medical response times across a city road network using graph algorithms.

When an emergency occurs at an incident node:
1. **Find Nearest Ambulance:** Calculates the shortest path from all available ambulance stations to the emergency site and dispatches the closest vehicle.
2. **Find Nearest Hospital:** Calculates the shortest path from the emergency location to the most suitable hospital facility for critical patient transfer.
3. **Visualize Mission:** Visualizes the full city graph, waypoints, and optimal two-leg trajectory on an interactive Leaflet.js map.

---

## 🛠️ Technology Stack

| Component | Technology | Role |
|---|---|---|
| **Core DSA Engine** | **C++17** | Graph representations, Min-Heap, Dijkstra's algorithm |
| **Backend API** | **Python Flask** | Lightweight HTTP service, routing orchestration |
| **Frontend UI** | **HTML5, CSS3, JavaScript** | Single-page control dashboard |
| **Map Engine** | **Leaflet.js** | Open-source map layer rendering nodes and routes |
| **Dataset & IPC** | **JSON** | Static simulated city graph and API communication |
| **Build System** | **CMake ($\ge$ 3.16)** | Cross-platform C++ compilation |

---

## 🚫 Project Constraints Satisfied

- ✅ **No SQL Database:** Data is statically defined in clean JSON files.
- ✅ **No Authentication / Login:** Pure educational simulation with immediate access.
- ✅ **No Real-Time GPS / External Traffic APIs:** Completely deterministic, reproducible, and offline-capable.
- ✅ **No CRUD Management System:** Focused purely on algorithmic optimization.
- ✅ **All Core DSA Implemented in C++:** Adjacency List graph and Dijkstra shortest path solver written in native C++17.

---

## 📁 Folder Structure

```
SmartResQ/
├── cpp_engine/              # Core C++17 DSA Engine
│   ├── include/
│   │   ├── Graph.hpp        # Adjacency list and node/edge declarations
│   │   └── Dijkstra.hpp     # Shortest path solver declarations
│   ├── src/
│   │   ├── Graph.cpp        # Graph implementation
│   │   ├── Dijkstra.cpp     # Priority-queue Dijkstra implementation
│   │   └── main.cpp         # Standalone CLI driver
│   ├── tests/
│   │   └── test_graph.cpp   # Automated unit tests
│   └── CMakeLists.txt       # CMake build definition
├── backend/
│   └── app.py               # Flask REST service
├── frontend/
│   ├── index.html           # Leaflet UI dashboard
│   ├── css/
│   │   └── style.css        # Clean dark-themed styling
│   └── js/
│       └── app.js           # Frontend controller
├── data/
│   ├── city_graph.json      # 18 locations, 29 bidirectional roads
│   ├── ambulances.json      # 3 ambulance units & base stations
│   └── hospitals.json       # 3 hospital facilities & bed capacities
├── docs/
│   ├── PROJECT_OVERVIEW.md    # Problem statement, objectives & proposed solution
│   ├── SYSTEM_ARCHITECTURE.md # Multi-tier IPC & sequence diagrams
│   ├── DSA_IMPLEMENTATION.md  # Detailed graph algorithms & C++ implementations
│   ├── COMPLEXITY_ANALYSIS.md # Master time/space complexity analysis table
│   ├── TESTING_REPORT.md      # Full verification results across 49 checks
│   ├── USER_GUIDE.md          # 10-step demonstration walkthrough & manual
│   ├── architecture.md        # Comprehensive system design spec
│   └── api_spec.md            # REST API & JSON communication specification
└── README.md                # Project documentation & run guide
```

---

## 🗺️ Simulated City Dataset

The project includes a realistic simulated metropolitan road network:
- **18 Locations:** Central Junction, General Hospital, Tech Blvd, Railway Station, University, Harbor, etc.
- **29 Roads:** Bidirectional segments with authentic distances in kilometers ($1.1\text{ km} - 3.6\text{ km}$).
- **3 Ambulances:** Stationed strategically at Station (`L5`), South Suburb (`L10`), and Heritage Gate (`L16`).
- **3 Hospitals:** City General (`L2`), Apex Trauma Center (`L8`), and Sunrise Specialty (`L14`).

---

## 🚀 Execution Instructions

### Prerequisites
- **C++ Compiler:** `g++` (GCC 9+ with C++17 support) or MSVC 2019/2022.
- **CMake:** Version 3.16 or higher.
- **Python:** Python 3.10+ with `Flask` and `flask-cors` installed:
  ```bash
  pip install Flask flask-cors
  ```

---

### Step 1: Build the C++ Core Engine (Windows)

Install MSYS2 and the UCRT64 compiler/build tools, then run these commands in
PowerShell from the project directory. Add the UCRT64 directory to `PATH` so
the engine and its runtime DLLs can be found:

```powershell
cd "C:\Users\Asus\OneDrive\Desktop\DSA_Project\SmartResQ"
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"

cmake -S .\cpp_engine -B .\cpp_engine\build -G Ninja
cmake --build .\cpp_engine\build
.\cpp_engine\build\smartresq_tests.exe
```

If MSYS2 was installed elsewhere, replace `C:\msys64\ucrt64\bin` with its
UCRT64 `bin` directory. CMake configuration is tied to the selected compiler;
if reconfiguring an existing build fails because its cached compiler is stale,
move `cpp_engine\build` aside and rerun the configure command.

---

### Step 2: Start the Flask Backend & Run Integration Tests

#### Run Automated Integration Tests:
Verify the API endpoints, C++ subprocess IPC, Dijkstra/BFS routing, and simulation state transitions:
```powershell
cd "C:\Users\Asus\OneDrive\Desktop\DSA_Project"
.\.venv\Scripts\Activate.ps1
python -m pip install Flask flask-cors
cd .\SmartResQ
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
python .\backend\test_api.py
```
Output:
```
====================================================================
   SmartResQ - Flask API & C++ Engine Integration Test Suite         
====================================================================
 [PASS] Test 01: GET /api/health
 [PASS] Test 02: GET /api/city
 [PASS] Test 03: GET /api/ambulances
 [PASS] Test 04: GET /api/hospitals
 [PASS] Test 05: POST /api/route (Dijkstra Shortest Path)
 [PASS] Test 06: POST /api/route (BFS Minimum-Hop Route)
 [PASS] Test 07: POST /api/route (Invalid Location Error Handling)
 [PASS] Test 08: POST /api/route (Missing Parameter Error Handling)
 [PASS] Test 09: POST /api/ambulance/select (First Dispatch - Nearest)
 [PASS] Test 10: POST /api/ambulance/select (Double-Allocation Prevention)
 [PASS] Test 11: GET /api/ambulances (State Reflection)
 [PASS] Test 12: POST /api/hospital/select (Hospital & Alternatives)
 [PASS] Test 13: POST /api/road/block (Simulate Incident on R01)
 [PASS] Test 14: GET /api/city (Road Blockage Reflection)
 [PASS] Test 15: POST /api/route (Dynamic Detour Recalculation)
 [PASS] Test 16: POST /api/road/reopen (Clear Incident on R01)
 [PASS] Test 17: POST /api/route (Route Baseline Restoration)
 [PASS] Test 18: POST /api/reset (Graph & Fleet Reset)
 [PASS] Test 19: Verification After Reset
====================================================================
  INTEGRATION TEST RESULTS: 19/19 TESTS PASSED (100.0%)
====================================================================
```

#### Start the Flask Web Server:
From the SmartResQ project directory in PowerShell:
```powershell
python .\backend\app.py
```

The Flask server will start on:
```
http://127.0.0.1:5000
```

Available API Endpoints:
- `GET  /api/health`            : Engine & dataset health check
- `GET  /api/city`              : Simulated city locations and roads (with dynamic blockage state)
- `GET  /api/ambulances`        : Ambulance fleet locations & availability
- `GET  /api/hospitals`         : Hospital facilities & emergency capacities
- `POST /api/route`             : Shortest path routing via native C++ Dijkstra or BFS
- `POST /api/ambulance/select`  : Nearest ambulance dispatch via native C++ Dijkstra
- `POST /api/hospital/select`   : Nearest hospital & ranked alternatives via native C++ Dijkstra
- `POST /api/road/block`        : Dynamic road closure simulation
- `POST /api/road/reopen`       : Road reopening & baseline restoration
- `POST /api/reset`             : Reset graph blockages and fleet availability

---

### Step 3: Open the Interactive Simulation Dashboard

- Open `http://127.0.0.1:5000` in your web browser (served directly by Flask), **OR**
- The dashboard is best opened at `http://127.0.0.1:5000` so it can call the Flask API.

#### Dashboard Capabilities:
1. **Interactive Leaflet City Map:**
   - 18 metropolitan locations rendered as clean graph nodes.
   - Blue ambulance markers (`#3b82f6`), Green hospital markers (`#10b981`), and Red incident marker (`#ef4444`).
   - Normal roads vs. dashed red blocked roads.
   - Clicking any node sets it as the active emergency incident.
   - Clicking any road toggles its blockage state in real time.
2. **Emergency Mission Dispatch:**
   - Select an incident node and trigger nearest ambulance dispatch and hospital selection.
   - Displays ambulance details, station, route distance, a clearly labeled 40 km/h simulated response estimate, hospital transfer metrics, and separate waypoints for both mission legs.
   - Draws Leg 1 (Ambulance $\to$ Incident) in amber and Leg 2 (Incident $\to$ Hospital) in cyan.
3. **Road Blockage Simulator:**
   - Review all 29 road segments, including endpoints, distance, and current status; block or reopen a road from the list or map.
   - Recalculates shortest detours natively via C++ Dijkstra with baseline vs. detour distance comparison.
4. **BFS vs. Dijkstra Algorithm Comparator:**
   - Select origin and destination to execute both Dijkstra ($O((V+E)\log V)$) and BFS ($O(V+E)$).
   - Side-by-side comparison cards displaying total distance, hop count, and full traversal paths.
   - Dedicated buttons to overlay Dijkstra (amber) or BFS (purple) routes directly on the map.
5. **One-Click Simulation Reset:**
   - Clears all road blockages and releases all dispatched ambulances back to available status.

---

## 🧪 DSA Verification Summary

The C++ unit test suite verifies all core graph algorithms, dispatch logic, and edge cases:
- **Test 1: Dijkstra Minimum-Distance Optimization:** Multi-route optimization, triangular shortcut selection.
- **Test 2: BFS Minimum-Hop vs. Dijkstra Minimum-Distance:** Verifies Dijkstra chooses distance-minimal path (6 km / 3 hops) while BFS chooses hop-minimal path (20 km / 1 hop).
- **Test 3: Equal-Distance Paths & Determinism:** Symmetric tie-breaking, valid path reconstruction.
- **Test 4: Road Blockages & Dynamic Rerouting:** Simulates traffic incidents, auto-reroutes around closures, and restores baseline paths upon reopening.
- **Test 5: Disconnected & Unreachable Destinations:** Safe handling of isolated nodes returning `found = false` and $\infty$ distance without crashing.
- **Test 6: Single-Node Route (Start == Destination):** Immediate local resolution ($0.0\text{ km}$, 0 hops, 1 node).
- **Test 7: DFS Traversal Order & Component Reachability:** Verifies deep-first recursive exploration and component isolation.
- **Test 8: Real City Dataset Emergency Routing (`city_graph.json`):** End-to-end multi-hop simulation over 18 metropolitan locations.
- **Test 9: Nearest Target Candidate Selection:** Multi-candidate optimization identifying the closest target.
- **Test 10: Ambulance Selection (Shortest Distance):** Dispatches nearest available ambulance unit.
- **Test 11: Double-Allocation Prevention:** Marks dispatched ambulances busy and prevents reallocation during the same simulation session.
- **Test 12: Unreachable Units Ignored:** Excludes stranded ambulances and hospitals on severed/disconnected components.
- **Test 13: Hospital Selection & Ranked Alternatives:** Identifies primary nearest hospital and returns sorted alternatives.
- **Test 14: End-to-End JSON City Simulation Dispatch:** End-to-end simulation loading all three JSON datasets (`city_graph.json`, `ambulances.json`, `hospitals.json`).
- **Test 15: Road Blockage Simulation - Detour & Distance Difference:** Simulates blocking road, recalculates route, and verifies exact distance delta (+2.50 km / +166.7%).
- **Test 16: Road Reopening & Immediate Restoration:** Reopens closed road and verifies instant restoration of baseline route.
- **Test 17: Bridge Road Blockage (No Alternate Route Exists):** Gracefully detects isolated destinations under bridge closure without crashing.
- **Test 18: BFS Hop-Minimal Detour Recalculation:** Verifies BFS recalculates detour with minimal road segments.
- **Test 19: Full Graph Reset Operation:** Restores all edges across multiple simultaneous road blockages.
- **Test 20: Real City Dataset Road Closure (R01 Blockage & Detour):** End-to-end simulation on metropolitan graph (1.20 km -> 3.10 km detour | Delta: +1.90 km | Restored).

Run the test suite at any time:
```bash
./SmartResQ/cpp_engine/build/smartresq_tests.exe
```
Expected output:
```
====================================================================
   SmartResQ - Full Verification Test Suite (Algorithms & Blockage) 
====================================================================
[Test 1/20] Dijkstra Minimum-Distance Optimization (Multiple Routes): PASSED (Optimal 5.50 km path chosen over 7.00 km and 12.00 km routes)
[Test 2/20] BFS Minimum Hops vs. Dijkstra Minimum Distance: PASSED (Dijkstra chose 6.0 km / 3 hops; BFS chose 20.0 km / 1 hop)
[Test 3/20] Equal-Distance Paths (Tie-Breaking & Integrity): PASSED (Deterministic 4.00 km resolution on equal paths)
[Test 4/20] Road Blockages & Dynamic Detour Rerouting: PASSED (Dynamic diversion 1.5 km -> 4.3 km detour -> 1.5 km restoration)
[Test 5/20] Disconnected & Unreachable Destinations: PASSED (Safe unreachable status returned without crash)
[Test 6/20] Single-Node Route (Start == Destination): PASSED (Immediate 0.0 km resolution for local origin)
[Test 7/20] DFS Reachability & Traversal Order: PASSED (DFS accurately tracks traversal order and component bounds)
[Test 8/20] Real City Dataset Emergency Routing (city_graph.json): PASSED (18 city nodes verified; Dijkstra distance: 4.1 km)
[Test 9/20] Nearest Target Candidate Selection: PASSED (Candidate T2 selected with 2.00 km minimum route)
[Test 10/20] Ambulance Selection (Shortest Distance): PASSED (Selected AMB_NEAR with 1.50 km travel distance)
[Test 11/20] Double-Allocation Prevention: PASSED (AMB_1 dispatched -> AMB_2 dispatched -> Double-allocation blocked)
[Test 12/20] Unreachable Units Ignored: PASSED (Unreachable units on severed components ignored)
[Test 13/20] Hospital Selection & Ordered Alternatives: PASSED (Primary H1 @ 2.0 km, Alternatives: H2 @ 5.0 km, H3 @ 8.0 km)
[Test 14/20] End-to-End JSON City Simulation Dispatch: PASSED (AMB-03 @ 1.7 km -> L9 -> HOSP-02 @ 1.8 km | Total: 3.50 km)
[Test 15/20] Road Blockage Simulation - Detour & Distance Difference: PASSED (Original: 1.50 km, Detour: 4.00 km, Delta: +2.50 km / +166.7%)
[Test 16/20] Road Reopening & Immediate Baseline Route Restoration: PASSED (Road R_AB reopened and baseline 1.00 km path immediately restored)
[Test 17/20] Bridge Road Blockage (No Alternate Route Exists): PASSED (No alternate route detected cleanly without crash)
[Test 18/20] BFS Hop-Minimal Detour Recalculation: PASSED (BFS recalculated detour: 1 hop -> 3 hops)
[Test 19/20] Full Graph Reset Operation (Multiple Road Blockages Restored): PASSED (3 simultaneous road blockages cleared by reset operation)
[Test 20/20] Real City Dataset Road Closure (R01 Blockage & Detour): PASSED (R01 blocked: 1.20 km -> 3.10 km detour | Delta: +1.90 km | Restored)
====================================================================
  ALL 20 UNIT TEST SUITES PASSED SUCCESSFULLY (100% PASS RATE)     
====================================================================
```
