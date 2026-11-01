# SmartResQ – Project Overview

> **Academic Course:** Data Structures and Algorithms (DSA)  
> **Target Audience:** Third-Year Computer Engineering Examination & Practical Evaluation  
> **Project Title:** SmartResQ – Emergency Ambulance Routing and Shortest Path Optimization Using Graph Algorithms  
> **Core Technologies:** C++17, Python Flask, HTML5/CSS3/JavaScript, Leaflet.js, CMake  

---

## 1. Problem Statement

In metropolitan emergency medical services (EMS), response latency directly influences patient survival rates. When an emergency incident occurs:
1. Dispatchers must rapidly identify the closest operational ambulance among distributed base stations.
2. The dispatched ambulance must traverse urban road networks via the path of least physical distance or travel time.
3. Sudden traffic bottlenecks, road closures, or infrastructure damage can sever optimal transit routes, necessitating immediate algorithmic detour computation.
4. Following on-scene stabilization, the patient must be routed to the nearest trauma-capable hospital facility while ranking alternatives if primary centers operate at full ICU capacity.

Commercial navigation suites depend on closed-source APIs, proprietary cloud dependencies, and heavy web frameworks. In academic and mission-critical offline environments, a lightweight, deterministic, and algorithmically transparent graph dispatch system implemented in native **C++17** provides optimal speed, predictable memory usage, and full control over core data structures.

---

## 2. Project Objectives

- **Algorithmic Rigor:** Implement core graph representations, priority queues, and pathfinding solvers natively in **C++17** without third-party graph packages (e.g., NetworkX or Boost.Graph).
- **Two-Leg Emergency Routing:** Automatically compute:
  - **Leg 1 (Ambulance Response):** Nearest available ambulance station $\to$ Emergency Incident site.
  - **Leg 2 (Hospital Transport):** Emergency Incident site $\to$ Closest reachable trauma facility.
- **Dynamic Graph Mutation:** Simulate realistic road closures by blocking and reopening edges on the fly, dynamically rerouting vehicles around obstacles using Dijkstra's algorithm.
- **Resource Lock & Allocation:** Prevent double-allocation of emergency vehicles during sequential emergency calls within the same operational cycle.
- **Comparative Analysis:** Compare weighted shortest-path optimization (**Dijkstra's Algorithm**) against unweighted minimum-hop traversal (**Breadth-First Search**).
- **Interactive Visualization:** Provide an intuitive, dark-themed dashboard powered by Leaflet.js that visualizes the simulated metropolitan network, vehicles, medical centers, and trajectory polylines.

---

## 3. Proposed Solution

SmartResQ is architectured as a decoupled, multi-tier system:

```
+-----------------------------------------------------------------+
|                      PRESENTATION LAYER                         |
|   HTML5 / CSS3 / Vanilla JavaScript / Leaflet.js Web Dashboard  |
+-----------------------------------------------------------------+
                                ||  HTTP / REST (JSON)
                                \/
+-----------------------------------------------------------------+
|                    ORCHESTRATION GATEWAY                        |
|    Python Flask Service (Lightweight Gateway & State Tracker)   |
+-----------------------------------------------------------------+
                                ||  Standard I/O Subprocess IPC
                                \/
+-----------------------------------------------------------------+
|                     CORE COMPUTATION ENGINE                     |
|           Native C++17 Graph DSA Engine (smartresq_engine)       |
|    - Adjacency List Graph (CityGraph)                           |
|    - Binary Min-Heap Priority Queue (std::priority_queue)       |
|    - Dijkstra Shortest Path Solver (O((V + E) log V))           |
|    - Breadth-First Search Solver (O(V + E))                     |
|    - Depth-First Search Reachability (O(V + E))                 |
|    - Dynamic Road Blockage & Rerouting Simulator                |
+-----------------------------------------------------------------+
```

### Architectural Guarantees:
- **Separation of Concerns:** Flask serves solely as a bridge and state coordinator; zero graph traversal or route computation takes place in Python.
- **Deterministic Complexity:** Dijkstra operates in $\mathcal{O}((V + E) \log V)$ time and BFS in $\mathcal{O}(V + E)$ time with bounded memory footprints.
- **Cross-Platform Compilation:** Built with CMake ($\ge 3.16$) and Ninja/Make supporting GCC (MinGW-w64), Clang, and MSVC.

---

## 4. Key Constraints Satisfied

| Constraint | Architectural Strategy | Benefit |
|---|---|---|
| **No SQL Database** | Datasets stored in structured JSON files (`data/`). | Immediate portability, zero database setup overhead. |
| **No Authentication / Login** | Direct dashboard access with immediate simulation readiness. | Frictionless evaluation and classroom demonstration. |
| **No Real-Time GPS / APIs** | Deterministic metropolitan coordinate model (`Metropolis Central`). | Completely offline-capable and reproducible results. |
| **No CRUD Management** | Focused purely on graph algorithms and emergency optimization. | Clean codebase free of boilerplate administrative forms. |
| **Native DSA Engine** | All graph structures and algorithms written in pure C++17. | Meets rigorous Computer Engineering curriculum standards. |

---

## 5. Simulated Metropolitan Dataset

The project models a realistic simulated metropolitan road network named **Metropolis Central**:
- **18 Urban Locations (Vertices $V$):**
  Landmarks including `Central Junction (L1)`, `City General Hospital (L2)`, `Tech Boulevard (L3)`, `Central Railway Station (L5)`, `University Campus (L6)`, `Apex Trauma Center (L8)`, and `Airport Expressway (L13)`.
- **29 Bidirectional Road Segments (Edges $E$):**
  Realistic segment distances ranging between $1.1\text{ km}$ and $3.6\text{ km}$, totaling 58 directed edges.
- **3 Strategic Ambulance Stations:**
  - `AMB-01` (*Ambulance Alpha*): Advanced Life Support at `L5 (Central Railway Station)`.
  - `AMB-02` (*Ambulance Bravo*): Basic Life Support at `L10 (South Suburb Hub)`.
  - `AMB-03` (*Ambulance Charlie*): Mobile ICU at `L16 (Heritage Gate)`.
- **3 Regional Hospital Facilities:**
  - `HOSP-01` (*City General Hospital*): Level 1 Regional Trauma Center at `L2` (15 ICU beds).
  - `HOSP-02` (*Apex Trauma Center*): Level 2 Emergency Trauma Unit at `L8` (12 ICU beds).
  - `HOSP-03` (*Sunrise Specialty Hospital*): Comprehensive Cardiac Center at `L14` (20 ICU beds).

---

## 6. Limitations and Future Scope

### Current Limitations:
1. **Static Speed Model:** Edge weights represent physical distance in kilometers. Variable speed limits, traffic signal delays, and live weather conditions are not factored into the weight equation.
2. **Single Patient per Ambulance:** Each ambulance carries one patient per mission run before returning to the available pool.
3. **Discrete Road Blockage:** Roads are either completely traversable or completely blocked; fractional capacity degradation (e.g. 50% slowdown) is not modeled.

### Future Scope:
1. **Dynamic Edge Cost Functions:** Incorporate time-varying cost functions:
   $$\text{Weight} = \frac{\text{Distance}}{\text{Speed Limit} \times \text{Congestion Factor}} + \text{Signal Delay}$$
2. **A\* Heuristic Search:** Augment Dijkstra with Euclidean or Haversine distance heuristics to enable $A^*$ search for large-scale continental graphs ($V > 100{,}000$).
3. **Multi-Vehicle Fleet Routing Problem (VRP):** Extend the dispatcher to solve the Capacitated Vehicle Routing Problem with Time Windows (CVRPTW) for mass-casualty incidents.
4. **Elevation & Gradient Penalties:** Factor topographic slope into route calculations for heavy emergency vehicles.

