# SmartResQ – JSON Communication & API Specification

**Project:** SmartResQ – Emergency Ambulance Routing and Shortest Path Optimization  
**Architecture:** Frontend (Leaflet.js) <--> Flask REST API <--> C++17 DSA Engine (Subprocess IPC / JSON)  
**Base URL:** `http://127.0.0.1:5000/api`  
**Version:** `1.1.0`

---

## 1. System Communication Architecture

SmartResQ strictly separates presentation, API orchestration, and algorithmic computation:
1. **Frontend Tier (HTML5 / CSS3 / JavaScript / Leaflet.js):**
   - Displays city locations and road network polylines.
   - Allows users to select emergency locations and click roads to block/unblock them.
   - Issues asynchronous `fetch()` HTTP requests to the Flask API.
2. **Backend Gateway (Python 3 / Flask / flask-cors):**
   - Exposes clean REST endpoints.
   - Validates client request schemas.
   - Coordinates simulation state across requests (maintaining active `blocked_roads` and `allocated_ambulances`).
   - Invokes the compiled native C++ engine via `subprocess.run([binary, "--json"], input=...)`.
3. **Core DSA Engine (C++17):**
   - Receives JSON requests via standard input (`stdin`).
   - Executes native graph algorithms: Dijkstra (Min-Heap), BFS (Queue), DFS, Nearest Candidate Selection.
   - Serializes computation results to standard output (`stdout`) as RFC 8259-compliant JSON.
   - **Zero graph or shortest-path calculations are performed in Python.**

```
+------------------+         HTTP / JSON         +--------------------+
|  Leaflet.js Web  |  <======================>   |   Python Flask     |
|     Frontend     |                             |      Backend       |
+------------------+                             +--------------------+
                                                           ||
                                                  stdin / stdout (IPC)
                                                  JSON Subprocess Pipe
                                                           ||
                                                           \/
                                                 +--------------------+
                                                 |  Native C++17 DSA  |
                                                 |    Graph Engine    |
                                                 | (Dijkstra / BFS /  |
                                                 | Road Blockage Sim) |
                                                 +--------------------+
```

---

## 2. Comprehensive API Endpoints

### 2.1 Health & Engine Status
- **Endpoint:** `GET /api/health`
- **Description:** Verifies service health, presence of the compiled C++ engine executable, dataset files, and active simulation metrics.
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "healthy",
  "service": "SmartResQ Emergency Dispatch Engine",
  "version": "1.0.0",
  "cpp_engine": {
    "available": true,
    "binary_path": "E:\\DSA_Project\\SmartResQ\\cpp_engine\\build\\smartresq_engine.exe"
  },
  "simulation_state": {
    "blocked_roads": [],
    "blocked_count": 0,
    "allocated_ambulances": [],
    "allocated_count": 0
  },
  "dataset_status": {
    "city_graph_available": true,
    "ambulances_available": true,
    "hospitals_available": true
  }
}
```

---

### 2.2 City Network Topology
- **Endpoint:** `GET /api/city`
- **Description:** Returns all city locations and road segments. Each road dynamically includes `is_blocked: boolean` reflecting current simulation state.
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "city_name": "Metropolis Central",
  "locations": [
    { "id": "L1", "name": "Central Junction", "latitude": 18.5204, "longitude": 73.8567 },
    { "id": "L2", "name": "City General Hospital", "latitude": 18.5285, "longitude": 73.8525 }
  ],
  "roads": [
    { "id": "R01", "source": "L1", "destination": "L2", "distance_km": 1.2, "is_blocked": false },
    { "id": "R02", "source": "L1", "destination": "L3", "distance_km": 2.1, "is_blocked": false }
  ],
  "summary": {
    "total_locations": 18,
    "total_roads": 29,
    "blocked_roads": []
  }
}
```

---

### 2.3 Ambulance Fleet Status
- **Endpoint:** `GET /api/ambulances`
- **Description:** Returns all deployed ambulances, their stations, and dynamically tracks single-dispatch availability.
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "ambulances": [
    {
      "id": "AMB-01",
      "name": "Ambulance Alpha",
      "location_id": "L5",
      "station_name": "Central Railway Station",
      "vehicle_type": "Advanced Life Support (ALS)",
      "is_available": true,
      "status": "Available",
      "contact": "+1-800-555-0191"
    }
  ],
  "allocated_ambulances": []
}
```

---

### 2.4 Hospital Facilities
- **Endpoint:** `GET /api/hospitals`
- **Description:** Returns hospital facilities, locations, and ICU trauma capacities.
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "hospitals": [
    {
      "id": "HOSP-01",
      "name": "City General Hospital",
      "location_id": "L2",
      "tier": "Level 1 Regional Trauma Center",
      "available_icu_beds": 15,
      "total_beds": 120,
      "emergency_helpline": "+1-800-555-0101"
    }
  ]
}
```

---

### 2.5 Shortest Route Calculation
- **Endpoint:** `POST /api/route`
- **Description:** Calculates the shortest route between two locations using native C++ Dijkstra (minimum distance) or BFS (minimum hops). Excludes blocked roads automatically.
- **Request Body:**
```json
{
  "origin": "L1",
  "destination": "L8",
  "algorithm": "dijkstra"
}
```
- **Response Code:** `200 OK` (or `400 Bad Request` if invalid vertex / parameter)
- **Response Example (Dijkstra):**
```json
{
  "status": "success",
  "found": true,
  "algorithm": "Dijkstra (Min-Heap Priority Queue)",
  "origin": "L1",
  "destination": "L8",
  "distance_km": 4.6000,
  "hop_count": 3,
  "path_nodes": ["L1", "L16", "L9", "L8"],
  "road_ids": ["R03", "R23", "R21"]
}
```

---

### 2.6 Nearest Ambulance Selection & Dispatch
- **Endpoint:** `POST /api/ambulance/select`
- **Description:** Dispatches the nearest available ambulance to an emergency location using native C++ Dijkstra. Automatically locks the ambulance to prevent double allocation.
- **Request Body:**
```json
{
  "incident_location_id": "L1"
}
```
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "success": true,
  "message": "Nearest ambulance allocated successfully.",
  "selected_ambulance": {
    "id": "AMB-03",
    "name": "Ambulance Charlie",
    "location_id": "L16"
  },
  "distance_km": 1.1000,
  "path_nodes": ["L16", "L1"],
  "road_ids": ["R03"],
  "allocated_ambulances": ["AMB-03"]
}
```

---

### 2.7 Nearest Hospital Selection & Ranked Alternatives
- **Endpoint:** `POST /api/hospital/select`
- **Description:** Evaluates all trauma centers from the patient location using native C++ Dijkstra. Selects the nearest reachable facility and ranks alternative hospitals with distances.
- **Request Body:**
```json
{
  "patient_location_id": "L1"
}
```
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "success": true,
  "message": "Nearest hospital identified with alternative facilities evaluated.",
  "selected_hospital": {
    "id": "HOSP-01",
    "name": "City General Hospital",
    "location_id": "L2"
  },
  "distance_km": 1.2000,
  "path_nodes": ["L1", "L2"],
  "road_ids": ["R01"],
  "alternatives": [
    {
      "id": "HOSP-02",
      "name": "Apex Trauma Center",
      "location_id": "L8",
      "distance_km": 4.6000
    },
    {
      "id": "HOSP-03",
      "name": "Sunrise Specialty Hospital",
      "location_id": "L14",
      "distance_km": 7.0000
    }
  ]
}
```

---

### 2.8 Dynamic Road Blockage Simulation
- **Endpoint:** `POST /api/road/block`
- **Description:** Simulates an accident or road blockage in the C++ graph.
- **Request Body:**
```json
{
  "road_id": "R01"
}
```
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "blocked_road_id": "R01",
  "is_connected": true,
  "message": "Road [R01] blocked successfully.",
  "all_blocked_roads": ["R01"]
}
```

---

### 2.9 Road Reopening & Clearance
- **Endpoint:** `POST /api/road/reopen`
- **Description:** Reopens a previously blocked road in the C++ graph and restores baseline route availability.
- **Request Body:**
```json
{
  "road_id": "R01"
}
```
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "reopened_road_id": "R01",
  "is_connected": true,
  "message": "Road [R01] reopened successfully.",
  "all_blocked_roads": []
}
```

---

### 2.10 Simulation Reset
- **Endpoint:** `POST /api/reset`
- **Description:** Clears all active road closures and releases all allocated ambulances back to "Available".
- **Request Body:** `{}`
- **Response Code:** `200 OK`
- **Response Example:**
```json
{
  "status": "success",
  "message": "Graph network and ambulance fleet reset to original state.",
  "all_blocked_roads": [],
  "allocated_ambulances": []
}
```
