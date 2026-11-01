# SmartResQ – Comprehensive Testing & Verification Report

> **Project:** SmartResQ – Emergency Ambulance Routing and Shortest Path Optimization  
> **Course:** Data Structures and Algorithms (DSA)  
> **Evaluation Date:** October 2026  
> **Test Status:** 100% PASSED (Zero Failures / Zero Regressions)  

---

## 1. Test Environment & Toolchain

| Component | Tool / Environment Specification |
|---|---|
| **Operating System** | Windows 11 Pro 64-bit |
| **C++ Compiler** | GCC 15.2.0 (`x86_64-w64-mingw32`, UCRT64) |
| **C++ Standard** | ISO C++17 (`-std=c++17 -O2`) |
| **Build Generator** | CMake 4.4.4 & Ninja 1.13.2 |
| **Python Environment** | Python 3.14.3 |
| **Web Service Framework** | Flask 3.1.3 & flask-cors 6.0.5 |
| **Frontend Test Harness** | Chromium Web Engine & Leaflet.js 1.9.4 |

---

## 2. Test Execution Summary

```
+-------------------------------------------------------------------------------+
|                             SMARTRESQ TEST MATRIX                             |
+------------------------------------+------------+----------+------------------+
| Test Suite                         | Executed   | Passed   | Success Rate     |
+------------------------------------+------------+----------+------------------+
| Native C++ DSA Unit Tests          | 20 tests   | 20       | 100.0%           |
| Flask REST API Integration Suite   | 19 tests   | 19       | 100.0%           |
| Master 10-Point System Battery     | 10 areas   | 10       | 100.0%           |
+------------------------------------+------------+----------+------------------+
| Cumulative Total                   | 49 checks  | 49       | 100.0%           |
+------------------------------------+------------+----------+------------------+
```

---

## 3. Native C++ Unit Test Results (`smartresq_tests.exe`)

Executable command:
```bash
./cpp_engine/build/smartresq_tests.exe
```

Log Output:
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

---

## 4. Master 10-Point Verification Battery (`verify_all.py`)

Executable command:
```bash
python backend/verify_all.py
```

### Detailed Evaluation Results:

#### 1. Graph Construction
- **Status:** `PASSED`
- **Verification Details:** Successfully loaded 18 vertices (`L1` to `L18`) and 29 bidirectional roads (`R01` to `R29`). Outgoing adjacency list correctly verified for bidirectional indexing (58 directed edge records). Geographic coordinate bounds validated in range Lat $[18.49, 18.56]$ and Lon $[73.82, 73.89]$.

#### 2. Dijkstra Shortest Path
- **Status:** `PASSED`
- **Verification Details:** Evaluated path from Central Junction (`L1`) to Apex Trauma Center (`L8`). Dijkstra with Min-Heap Priority Queue correctly selected the optimal path:
  `L1` $\to$ `L16` $\to$ `L9` $\to$ `L8` (via roads `R03`, `R23`, `R21`) totaling exactly **$4.60\text{ km}$** across 3 hops.

#### 3. BFS Minimum-Hop Path
- **Status:** `PASSED`
- **Verification Details:** Evaluated path from `L1` to `L8` using BFS with FIFO Queue. Correctly identified path with fewest road segments (3 hops) and reconstructed sequence `L1` $\to$ `L16` $\to$ `L9` $\to$ `L8`.

#### 4. DFS Connectivity Traversal
- **Status:** `PASSED`
- **Verification Details:** Traversed Metropolis Central starting from central node `L1`. DFS reached all 18 vertices without encountering unvisited subgraphs, confirming complete initial network connectivity (`is_connected: true`).

#### 5. Nearest Ambulance Selection & Double-Dispatch Prevention
- **Status:** `PASSED`
- **Verification Details:**
  - *Call 1:* Emergency at `L1`. System evaluated available ambulances:
    `AMB-01` (Station `L5`, $3.5\text{ km}$), `AMB-02` (Station `L10`, $4.7\text{ km}$), and `AMB-03` (Station `L16`, $1.1\text{ km}$).
    System dispatched **`AMB-03`** (closest at $1.10\text{ km}$).
  - *Call 2:* Immediate second emergency at `L1`. `AMB-03` was locked (`is_available = false`). System automatically selected next nearest available unit **`AMB-01`** ($3.50\text{ km}$), preventing double-booking.

#### 6. Nearest Hospital Selection & Ranked Alternatives
- **Status:** `PASSED`
- **Verification Details:** Evaluated hospital choices for patient at `L1`:
  - Selected Primary: `City General Hospital (HOSP-01)` at `L2` ($1.20\text{ km}$).
  - Ranked Alternative 1: `Apex Trauma Center (HOSP-02)` at `L8` ($4.60\text{ km}$).
  - Ranked Alternative 2: `Sunrise Specialty Hospital (HOSP-03)` at `L14` ($7.00\text{ km}$).

#### 7. Road Blockage Simulation
- **Status:** `PASSED`
- **Verification Details:** Dispatched `POST /api/road/block` with `road_id = "R01"`. The edge was flagged `is_blocked = true` in memory. Verified via `GET /api/city` that road `R01` dynamically reflected `is_blocked = true`.

#### 8. Route Recalculation & Detour Analysis
- **Status:** `PASSED`
- **Verification Details:**
  - Baseline route `L1` $\to$ `L2` was direct via `R01` ($1.20\text{ km}$, 1 hop).
  - With `R01` blocked, Dijkstra re-routed through `L1` $\to$ `L17` $\to$ `L2` (via roads `R04`, `R06`), distance **$3.10\text{ km}$** (2 hops).
  - Distance delta accurately verified: **$+1.90\text{ km}$ (+158.3% detour)**.

#### 9. Unreachable Destination & Severed Component Handling
- **Status:** `PASSED`
- **Verification Details:** Blocked all four egress roads from `L1` (`R01`, `R02`, `R03`, `R04`). A subsequent route query `L1` $\to$ `L8` returned `found: false`, distance code `-1.0`, and empty path nodes `[]`. The system handled total isolation safely with zero crashes or exceptions.

#### 10. Simulation Reset & Baseline Restoration
- **Status:** `PASSED`
- **Verification Details:** Dispatched `POST /api/reset`. All blocked roads were restored (`blocked_roads = []`), all ambulance allocations were cleared (`allocated_ambulances = []`), and the direct route `L1` $\to$ `L2` immediately returned to the $1.20\text{ km}$ baseline.

---

## 5. API Endpoints Verification Matrix

| Endpoint | Method | Input Parameters | Expected Response | Verified Code | Status |
|---|---|---|---|---|---|
| `/api/health` | `GET` | None | Service & C++ engine status | `200 OK` | `PASS` |
| `/api/city` | `GET` | None | 18 nodes, 29 roads with `is_blocked` | `200 OK` | `PASS` |
| `/api/ambulances` | `GET` | None | 3 units with `is_available` flag | `200 OK` | `PASS` |
| `/api/hospitals` | `GET` | None | 3 hospitals with ICU bed counts | `200 OK` | `PASS` |
| `/api/route` | `POST` | `{"origin":"L1","destination":"L8"}` | Dijkstra shortest path result | `200 OK` | `PASS` |
| `/api/route` (BFS) | `POST` | `{"origin":"L1","destination":"L8","algorithm":"bfs"}` | BFS hop-minimal result | `200 OK` | `PASS` |
| `/api/route` (Err) | `POST` | `{"destination":"L2"}` | Missing origin validation error | `400 Bad Request` | `PASS` |
| `/api/ambulance/select`| `POST` | `{"incident_location_id":"L1"}` | Nearest unit + distance | `200 OK` | `PASS` |
| `/api/hospital/select` | `POST` | `{"patient_location_id":"L1"}` | Primary hospital + alternatives | `200 OK` | `PASS` |
| `/api/road/block` | `POST` | `{"road_id":"R01"}` | Confirmation + blocked list | `200 OK` | `PASS` |
| `/api/road/reopen` | `POST` | `{"road_id":"R01"}` | Confirmation + reopened list | `200 OK` | `PASS` |
| `/api/reset` | `POST` | `{}` | Confirmation + cleared state | `200 OK` | `PASS` |

