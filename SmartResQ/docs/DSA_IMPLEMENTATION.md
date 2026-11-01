# SmartResQ – Data Structures & Algorithms (DSA) Implementation

> **Module:** Core C++17 Graph DSA Engine  
> **Course:** Data Structures and Algorithms (Third-Year Computer Engineering)  
> **Implementation Standard:** Pure C++17, Zero External Packages  

---

## 1. Graph Data Structure: Weighted Adjacency List

### 1.1 Architectural Rationale (Adjacency List vs. Adjacency Matrix)
In metropolitan road networks, vertices (junctions) possess small constant degrees (typically 2 to 4 road connections per intersection), rendering the network **extremely sparse**.

For our Metropolis Central dataset:
- Locations ($V$): $18$
- Roads ($E$): $29$ bidirectional edges ($58$ directed edges)
- Total possible edges in complete directed graph: $V(V - 1) = 18 \times 17 = 306$
- **Network Sparsity:**
  $$\text{Sparsity} = 1 - \frac{58}{306} \approx 81.05\%$$

An **Adjacency Matrix** would consume $\mathcal{O}(V^2) = 18 \times 18 = 324$ matrix entries, requiring $\mathcal{O}(V)$ time to iterate through neighbors of a single node.  
An **Adjacency List** consumes only $\mathcal{O}(V + E)$ memory and permits neighbor exploration in $\mathcal{O}(\text{deg}(u))$ time, which is optimal for priority-queue-based Dijkstra.

### 1.2 Data Structures Definition
Implemented in [`cpp_engine/include/Graph.hpp`](file:///E:/DSA_Project/SmartResQ/cpp_engine/include/Graph.hpp):

```cpp
// Directed weighted road edge
struct RoadEdge {
    std::string id;             // Unique identifier (e.g. "R01")
    std::string source;         // Origin node ID (e.g. "L1")
    std::string destination;    // Target node ID (e.g. "L2")
    double distance_km{0.0};    // Edge weight in kilometers
    bool is_blocked{false};     // Dynamic blockage flag
};

// City location vertex
struct LocationNode {
    std::string id;             // Unique ID (e.g. "L1")
    std::string name;           // Descriptive name (e.g. "Central Junction")
    double latitude{0.0};       // Geographical latitude
    double longitude{0.0};      // Geographical longitude
    std::vector<RoadEdge> edges;// Outgoing adjacency list
};
```

---

## 2. Dijkstra's Algorithm (Minimum Distance Route)

### 2.1 Algorithmic Theory
Dijkstra's Algorithm finds the shortest path between a designated source vertex $s$ and all other reachable vertices in a directed, non-negatively weighted graph.

### 2.2 Min-Heap Priority Queue Optimization
Instead of a naive array with linear search ($\mathcal{O}(V^2)$), SmartResQ implements a **Binary Min-Heap Priority Queue** using C++ standard template library:
```cpp
using PQElement = std::pair<double, std::string>; // (distance_from_source, node_id)
std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> min_heap;
```
This reduces the vertex extraction operation from $\mathcal{O}(V)$ to $\mathcal{O}(\log V)$.

### 2.3 Relaxation Logic with Dynamic Blockage Exclusion
For every edge $(u, v)$ outgoing from the current minimum-distance vertex $u$:
$$\text{If } \neg e.\text{is\_blocked} \land \text{dist}[u] + w(u, v) < \text{dist}[v] \implies \text{dist}[v] \leftarrow \text{dist}[u] + w(u, v)$$

```cpp
while (!min_heap.empty()) {
    auto [current_dist, u] = min_heap.top();
    min_heap.pop();

    if (visited.count(u)) continue;
    visited.insert(u);

    if (u == destination_id) break; // Destination reached early

    for (const auto& edge : graph.getOutgoingEdges(u)) {
        if (edge.is_blocked) continue; // Exclude blocked roads dynamically

        const std::string& v = edge.destination;
        double new_dist = current_dist + edge.distance_km;

        if (new_dist < dist[v]) {
            dist[v] = new_dist;
            parent[v] = {u, edge.id}; // Record predecessor and edge for path reconstruction
            min_heap.push({new_dist, v});
        }
    }
}
```

### 2.4 Path & Edge Reconstruction
Backtracks from `destination_id` to `source_id` via the `parent` map, reversing the sequence to produce the exact chronological journey:
```cpp
std::vector<std::string> path_nodes;
std::vector<std::string> road_ids;
std::string curr = destination_id;

while (curr != source_id) {
    auto [prev_node, road_id] = parent[curr];
    path_nodes.push_back(curr);
    road_ids.push_back(road_id);
    curr = prev_node;
}
path_nodes.push_back(source_id);
std::reverse(path_nodes.begin(), path_nodes.end());
std::reverse(road_ids.begin(), road_ids.end());
```

---

## 3. Breadth-First Search (BFS – Minimum Hop Route)

### 3.1 Algorithmic Theory
BFS traverses the graph level by level using a **First-In, First-Out (FIFO) Queue**. While Dijkstra optimizes the *sum of physical kilometers*, BFS optimizes the *number of road segments (hops)* traversed, irrespective of individual road lengths.

### 3.2 Implementation Details
```cpp
std::queue<std::string> q;
std::unordered_set<std::string> visited;
std::unordered_map<std::string, std::pair<std::string, std::string>> parent;

q.push(source_id);
visited.insert(source_id);

while (!q.empty()) {
    std::string u = q.front();
    q.pop();

    if (u == destination_id) break;

    for (const auto& edge : graph.getOutgoingEdges(u)) {
        if (edge.is_blocked) continue;

        const std::string& v = edge.destination;
        if (!visited.count(v)) {
            visited.insert(v);
            parent[v] = {u, edge.id};
            q.push(v);
        }
    }
}
```

---

## 4. Depth-First Search (DFS – Connectivity & Traversal)

DFS recursively visits unvisited neighbors to explore full topological depth. In SmartResQ, DFS verifies whether every city location remains reachable from the central hub:
```cpp
void dfsRecursive(const CityGraph& graph, const std::string& u,
                  std::unordered_set<std::string>& visited,
                  std::vector<std::string>& order) {
    visited.insert(u);
    order.push_back(u);

    for (const auto& edge : graph.getOutgoingEdges(u)) {
        if (!edge.is_blocked && !visited.count(edge.destination)) {
            dfsRecursive(graph, edge.destination, visited, order);
        }
    }
}
```
If $|visited| = |V|$, the graph is strongly connected under current operational conditions.

---

## 5. Emergency Resource Dispatch & Selection

### 5.1 Nearest Ambulance Selection (Multi-Source Evaluation)
1. Query all ambulances where `is_available == true`.
2. For each candidate ambulance $A_i$ stationed at location $L(A_i)$:
   - Execute Dijkstra from $L(A_i)$ to `incident_location_id`.
   - Discard candidates where `found == false` (e.g. severed road components).
3. Select ambulance $A^*$ minimizing travel distance:
   $$A^* = \arg\min_{A_i} \text{DijkstraDistance}(L(A_i), \text{incident})$$
4. **Double-Allocation Prevention:** Set `A^*->is_available = false` in the dispatch registry.

### 5.2 Nearest Hospital Selection & Ranked Alternatives
1. Query all hospital facilities $H_1, H_2, \dots, H_k$.
2. Compute shortest paths from patient location $P$ to each hospital location $L(H_j)$.
3. Select primary hospital $H^* = \arg\min_j \text{dist}(P, L(H_j))$.
4. Sort remaining reachable hospitals by ascending distance into the `alternatives` array.

---

## 6. Dynamic Road Blockage & Detour Simulation

When road segment $R_k$ is blocked:
1. Mutate graph edge:
   ```cpp
   city_graph.blockRoad("R01"); // Sets edge.is_blocked = true for forward & reverse edges
   ```
2. Subsequent Dijkstra calls prune blocked edges during neighbor relaxation.
3. If an alternate path exists:
   $$\Delta d = d_{\text{detour}} - d_{\text{original}} \ge 0$$
4. If all alternative paths are severed:
   The algorithm returns `found = false` with safe code `-1.0` without crashing.
5. Reopening restores `is_blocked = false` instantly restoring the baseline path.

