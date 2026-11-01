# SmartResQ – System Architecture Specification

> **Project:** SmartResQ – Emergency Ambulance Routing and Shortest Path Optimization  
> **Module:** System Architecture & Inter-Process Communication (IPC)  
> **Protocol:** JSON over Subprocess Pipes (IPC) & HTTP REST  

---

## 1. High-Level Architectural Model

SmartResQ follows a decoupled three-tier architectural pattern designed to combine native execution speed with web-based interactivity:

```mermaid
flowchart TD
    subgraph Tier1["Presentation Tier (Frontend)"]
        UI["Leaflet.js Interactive Map & Control Deck"]
        UI_Events["User Actions: Select Node, Block Road, Compare"]
    end

    subgraph Tier2["Orchestration Tier (Flask Gateway)"]
        API["Flask REST API (app.py)"]
        State["Simulation State Tracker\n(blocked_roads, allocated_ambulances)"]
        IPC["Subprocess Process Manager\n(stdin / stdout Pipe)"]
    end

    subgraph Tier3["Computation Tier (C++17 DSA Core)"]
        Engine["smartresq_engine.exe --json"]
        Parser["Lightweight Zero-Dep JsonParser"]
        Graph["CityGraph (Adjacency List)"]
        Solvers["GraphAlgorithms (Dijkstra, BFS, DFS)"]
        Dispatcher["EmergencyDispatcher & RoadBlockageSimulator"]
    end

    UI -->|HTTP POST/GET JSON| API
    API --> State
    API -->|Payload + State in JSON via stdin| IPC
    IPC -->|Execute with Subprocess Pipe| Engine
    Engine --> Parser
    Parser --> Graph
    Graph --> Dispatcher
    Dispatcher --> Solvers
    Solvers -->|RFC 8259 JSON via stdout| IPC
    IPC -->|Response JSON| API
    API -->|HTTP 200/400 JSON| UI
```

---

## 2. Decoupled Multi-Tier Subsystems

### 2.1 Presentation Tier (Client Dashboard)
- **Host:** Modern web browser (Chrome, Edge, Firefox).
- **Core Technologies:** HTML5, CSS3, Vanilla JavaScript (ES6+), Leaflet.js 1.9.4.
- **Responsibilities:**
  - Render CartoDB dark matter tiles and geographical node coordinates.
  - Draw city road polylines with interactive blockage tooltips.
  - Capture user selections (emergency locations, road blockage toggles, algorithm comparison triggers).
  - Issue asynchronous `fetch()` calls to the backend API.
  - Dynamically animate and overlay route waypoints, detours, and comparison statistics.

### 2.2 Orchestration Tier (Flask Gateway)
- **Host:** Python 3.10+ runtime with `Flask` and `flask-cors`.
- **Responsibilities:**
  - Expose 10 clean, standardized REST endpoints.
  - Validate client request payloads (ensuring required IDs are non-empty and well-formed).
  - Maintain session simulation state across stateless subprocess invocations:
    - Active road blockages (`simulation_state["blocked_roads"]`).
    - Allocated ambulance units (`simulation_state["allocated_ambulances"]`).
  - Thread-safe state synchronization using `threading.Lock`.
  - Invoke `smartresq_engine.exe --json` via `subprocess.run(..., input=json_str, capture_output=True)`.

### 2.3 Computation Tier (C++17 Engine)
- **Host:** Compiled native binary (`smartresq_engine.exe`).
- **Dependencies:** C++17 standard library only (`<vector>`, `<unordered_map>`, `<queue>`, `<cmath>`, etc.). Zero external package dependencies.
- **Responsibilities:**
  - Fast, memory-safe JSON parsing via [`JsonUtils.hpp`](file:///E:/DSA_Project/SmartResQ/cpp_engine/include/JsonUtils.hpp).
  - Dynamic graph reconstruction and edge blockage flags.
  - Dijkstra shortest path solver using `std::priority_queue` with Min-Heap comparator.
  - Breadth-First Search (BFS) route solver using `std::queue`.
  - Depth-First Search (DFS) component traversal using recursion.
  - Multi-candidate distance minimization for ambulance and hospital selection.
  - Emitting compact, valid RFC 8259 JSON strings to standard output.

---

## 3. Inter-Process Communication (IPC) Protocol

Communication between Python Flask and the native C++ engine uses standard input/output pipes with strict JSON payloads.

### 3.1 Sequence Diagram: Emergency Dispatch Transaction

```mermaid
sequenceDiagram
    autonumber
    actor User as Operator / User
    participant Frontend as Leaflet Dashboard
    participant Flask as Flask Gateway (app.py)
    participant Cpp as C++ Engine (smartresq_engine)

    User->>Frontend: Selects Incident Location [L1]
    User->>Frontend: Clicks "Dispatch Nearest Ambulance"
    Frontend->>Flask: POST /api/ambulance/select {"incident_location_id": "L1"}
    Flask->>Flask: Acquire State Lock & Attach active blocked_roads
    Flask->>Cpp: Spawns subprocess with JSON payload on stdin
    Note over Cpp: 1. Parse JSON Request<br/>2. Re-apply active blocked roads<br/>3. Re-apply allocated fleet<br/>4. Run Dijkstra from all available ambulances<br/>5. Select minimum distance unit [AMB-03]
    Cpp-->>Flask: Serialized JSON Result on stdout
    Flask->>Flask: Update simulation_state (allocated_ambulances += AMB-03)
    Flask-->>Frontend: HTTP 200 {"success": true, "selected_ambulance": "AMB-03", ...}
    
    Frontend->>Flask: POST /api/hospital/select {"patient_location_id": "L1"}
    Flask->>Cpp: Spawns subprocess with JSON payload on stdin
    Note over Cpp: 1. Run Dijkstra to all 3 hospitals<br/>2. Select nearest [HOSP-01]<br/>3. Rank alternatives [HOSP-02, HOSP-03]
    Cpp-->>Flask: Serialized JSON Result on stdout
    Flask-->>Frontend: HTTP 200 {"selected_hospital": "HOSP-01", "alternatives": [...]}
    Frontend->>Frontend: Render Leg 1 (Amber) & Leg 2 (Cyan) Polylines on Map
```

---

## 4. End-to-End Simulation State Coordination

Because `subprocess.run` spawns an isolated operating system process for each computation, the C++ binary executes statelessly. To preserve road closures and prevent double-dispatches across multiple user interactions, Flask acts as the persistent coordinator:

```
                  +----------------------------------+
                  |  Flask Memory: simulation_state  |
                  |  - blocked_roads: {"R01"}        |
                  |  - allocated_ambs: {"AMB-03"}    |
                  +----------------------------------+
                                   |
                  Inject state into outgoing JSON payload
                                   |
                                   v
+-----------------------------------------------------------------------+
|  Request JSON to C++:                                                 |
|  {                                                                    |
|    "action": "route",                                                 |
|    "origin": "L1",                                                    |
|    "destination": "L2",                                               |
|    "blocked_roads": ["R01"],                                          |
|    "allocated_ambulances": ["AMB-03"]                                 |
|  }                                                                    |
+-----------------------------------------------------------------------+
                                   |
                                   v
+-----------------------------------------------------------------------+
|  C++ Engine Execution:                                                |
|  1. graph.reopenAllRoads();                                           |
|  2. dispatcher.resetFleetAvailability();                              |
|  3. for (r : blocked_roads) graph.blockRoad(r);                       |
|  4. for (a : allocated_ambs) dispatcher.setAvailability(a, false);   |
|  5. Compute Dijkstra shortest path excluding blocked edges!           |
+-----------------------------------------------------------------------+
```

This design eliminates race conditions, avoids shared-memory file thrashing, and guarantees that every computation runs in a clean, reproducible C++ memory space.

---

## 5. Security & Boundary Controls

1. **Input Sanitization:** All vertex and road IDs are strictly validated. If a client submits a non-existent ID (e.g. `INVALID_NODE`), the C++ engine immediately intercepts it and returns a structured 400 error without segmentation faults:
   ```json
   { "status": "error", "error": "Destination location ID 'INVALID_NODE' not found in graph.", "code": 400 }
   ```
2. **Infinite Weight Protection:** When a destination is unreachable due to road closures, C++ serializes `distance_km = -1.0` (avoiding non-standard `inf` tokens) and returns `found: false`.
3. **Execution Timeout:** All C++ subprocess calls in Python enforce a strict 10-second threshold (`timeout=10`), guarding against CPU hangs or infinite loops.

