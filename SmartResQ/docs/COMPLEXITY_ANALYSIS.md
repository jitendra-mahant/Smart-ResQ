# SmartResQ – Time & Space Complexity Analysis

> **Academic Course:** Data Structures and Algorithms (DSA)  
> **Topic:** Asymptotic Complexity Bounds & Resource Profiling  
> **Notation:**  
> - $V$ = Number of Graph Vertices (Locations, $V = 18$)  
> - $E$ = Number of Directed Road Edges ($E = 58$)  
> - $K_A$ = Number of Ambulance Units ($K_A = 3$)  
> - $K_H$ = Number of Hospital Facilities ($K_H = 3$)  

---

## 1. Master Complexity Analysis Table

| Operation / Algorithm | Best-Case Time | Average-Case Time | Worst-Case Time | Auxiliary Space | Key Data Structure |
|---|---|---|---|---|---|
| **Graph Representation** | $\mathcal{O}(1)$ edge insert | $\mathcal{O}(1)$ edge insert | $\mathcal{O}(V + E)$ full load | $\mathcal{O}(V + E)$ | Adjacency List (`unordered_map` + `vector`) |
| **Dijkstra's Algorithm** | $\mathcal{O}(1)$ ($s = d$) | $\mathcal{O}((V + E) \log V)$ | $\mathcal{O}((V + E) \log V)$ | $\mathcal{O}(V)$ | Binary Min-Heap Priority Queue |
| **Breadth-First Search (BFS)** | $\mathcal{O}(1)$ ($s = d$) | $\mathcal{O}(V + E)$ | $\mathcal{O}(V + E)$ | $\mathcal{O}(V)$ | FIFO Queue (`std::queue`) |
| **Depth-First Search (DFS)** | $\mathcal{O}(V + E)$ | $\mathcal{O}(V + E)$ | $\mathcal{O}(V + E)$ | $\mathcal{O}(V)$ stack | Recursive Call Stack / Set |
| **Binary Min-Heap (PQ)** | $\mathcal{O}(1)$ `top()` | $\mathcal{O}(\log N)$ `push`/`pop` | $\mathcal{O}(\log N)$ `push`/`pop` | $\mathcal{O}(N)$ | Array-backed Complete Binary Tree |
| **Ambulance Selection** | $\mathcal{O}((V + E) \log V)$ | $\mathcal{O}(K_A(V + E) \log V)$ | $\mathcal{O}(K_A(V + E) \log V)$ | $\mathcal{O}(V)$ | Multi-source Dijkstra + Min Filter |
| **Hospital Selection** | $\mathcal{O}((V + E) \log V)$ | $\mathcal{O}((V + E) \log V + K_H \log K_H)$ | $\mathcal{O}((V + E) \log V + K_H \log K_H)$ | $\mathcal{O}(V + K_H)$ | Single-source Dijkstra + Sort |
| **Road Block / Reopen** | $\mathcal{O}(1)$ lookup | $\mathcal{O}(\text{deg}(u))$ search | $\mathcal{O}(\text{deg}(u))$ edge update | $\mathcal{O}(1)$ | In-place boolean flag |

---

## 2. In-Depth Algorithmic Derivations

### 2.1 Graph Representation (Adjacency List)
- **Time Complexity:**
  - `addLocation(LocationNode)`: $\mathcal{O}(1)$ hash map insertion.
  - `addRoad(RoadEdge)`: $\mathcal{O}(1)$ hash lookup + $\mathcal{O}(1)$ amortized `vector::push_back`.
  - `getOutgoingEdges(u)`: $\mathcal{O}(1)$ reference lookup returning vector of size $\text{deg}(u)$.
  - Neighbor iteration for vertex $u$: Exactly $\mathcal{O}(\text{deg}(u))$ steps.
- **Space Complexity:**
  - $V$ location records stored in `std::unordered_map<std::string, LocationNode>`.
  - Exactly $2E$ directed edge structs across all adjacency lists.
  - **Total Space:** $\Theta(V + E)$.

---

### 2.2 Dijkstra's Algorithm with Binary Min-Heap
Let $G = (V, E)$ be a weighted directed graph with non-negative edge weights.

#### Step-by-Step Derivation:
1. **Initialization:**
   - Set $\text{dist}[s] = 0$ and $\text{dist}[v] = \infty$ for all $v \neq s$: $\mathcal{O}(V)$.
   - Insert source $(0, s)$ into priority queue: $\mathcal{O}(\log 1) = \mathcal{O}(1)$.
2. **Min-Heap Extractions (`pop`):**
   - Each reachable vertex is extracted from the priority queue at most once as the settled minimum:
     $$V \text{ extractions} \times \mathcal{O}(\log V) = \mathcal{O}(V \log V)$$
3. **Edge Relaxations & Heap Insertions (`push`):**
   - For every vertex $u$, we inspect all outgoing edges $(u, v)$.
   - Across the entire execution, the inner loop executes $\sum_{u \in V} \text{deg}(u) = E$ times.
   - In the worst case, every edge relaxation triggers a `min_heap.push()`:
     $$E \text{ insertions} \times \mathcal{O}(\log V) = \mathcal{O}(E \log V)$$
4. **Path Reconstruction:**
   - Backtracking from destination $d$ to source $s$ takes at most $V$ predecessor lookups: $\mathcal{O}(V)$.

#### Total Time Complexity:
$$T(V, E) = \mathcal{O}(V) + \mathcal{O}(V \log V) + \mathcal{O}(E \log V) + \mathcal{O}(V) = \mathbf{\mathcal{O}((V + E) \log V)}$$

#### Auxiliary Space Complexity:
- `dist` hash map: $\mathcal{O}(V)$
- `parent` hash map: $\mathcal{O}(V)$
- `visited` hash set: $\mathcal{O}(V)$
- `min_heap` priority queue: at most $\mathcal{O}(E)$ elements if every edge pushes an updated candidate before extraction. Since $E \le V^2$, $\log E = \mathcal{O}(\log V)$ and queue memory is bounded by $\mathcal{O}(E)$.
- **Total Space:** $\mathbf{\mathcal{O}(V + E)}$.

---

### 2.3 Breadth-First Search (BFS)
BFS explores the graph in unweighted topological layers:
1. Every vertex enters `std::queue<std::string>` at most once: $V \times \mathcal{O}(1) = \mathcal{O}(V)$.
2. When vertex $u$ is dequeued, all its outgoing edges are scanned: $\sum_{u \in V} \text{deg}(u) = E$.
3. Queue push/pop operations take strictly $\mathcal{O}(1)$ time.

#### Total Time Complexity:
$$T_{\text{BFS}}(V, E) = \mathbf{\mathcal{O}(V + E)}$$

#### Auxiliary Space Complexity:
- `visited` set: $\mathcal{O}(V)$
- `parent` map: $\mathcal{O}(V)$
- `queue`: holds at most the maximum breadth of the graph $\le V$.
- **Total Space:** $\mathbf{\mathcal{O}(V)}$.

---

### 2.4 Depth-First Search (DFS)
DFS explores as deeply as possible along each branch before backtracking:
1. Every vertex is marked visited once: $\mathcal{O}(V)$.
2. Every edge is traversed once: $\mathcal{O}(E)$.
3. Recursion call stack depth in the worst case (a linear graph) is $V$.

#### Total Time Complexity:
$$T_{\text{DFS}}(V, E) = \mathbf{\mathcal{O}(V + E)}$$

#### Auxiliary Space Complexity:
- Call stack: $\mathcal{O}(V)$
- Visited set: $\mathcal{O}(V)$
- **Total Space:** $\mathbf{\mathcal{O}(V)}$.

---

### 2.5 Binary Min-Heap Priority Queue Mechanics
Implemented via `std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>>`:
- **Underlying Structure:** Complete binary tree stored inside a contiguous dynamic array (`std::vector`).
- **Parent-Child Index Relationships (0-indexed):**
  - $\text{Parent}(i) = \lfloor \frac{i - 1}{2} \rfloor$
  - $\text{LeftChild}(i) = 2i + 1$
  - $\text{RightChild}(i) = 2i + 2$
- **Operation Complexities:**
  - `top()`: Returns root element at index 0 $\implies \mathcal{O}(1)$.
  - `push(val)`: Appends element to vector end, then sift-up along tree height $\le \lfloor \log_2 N \rfloor \implies \mathcal{O}(\log N)$.
  - `pop()`: Swaps root with last element, pops vector end, then sift-down along tree height $\le \lfloor \log_2 N \rfloor \implies \mathcal{O}(\log N)$.

---

### 2.6 Emergency Ambulance Selection
- Given $K_A$ candidate ambulances ($K_A = 3$):
  - For each available ambulance $A_i$, the system runs Dijkstra from $L(A_i)$ to incident $P$.
  - Number of Dijkstra invocations: $K_A$.
  - Total selection time:
    $$T_{\text{amb}} = K_A \times \mathcal{O}((V + E) \log V)$$
  - For Metropolis Central ($K_A = 3, V = 18, E = 58$):
    $$3 \times (18 + 58) \log_2(18) \approx 3 \times 76 \times 4.17 \approx 950 \text{ operations (virtually instantaneous, } < 0.2\text{ ms)}$$
- **Auxiliary Space:** $\mathcal{O}(V + E)$ reused per query.

---

### 2.7 Hospital Selection & Ranked Alternatives
- Given patient location $P$ and $K_H$ hospitals ($K_H = 3$):
  1. A single Dijkstra run from $P$ computes shortest paths to all vertices: $\mathcal{O}((V + E) \log V)$.
  2. Extract distance for each hospital: $K_H \times \mathcal{O}(1) = \mathcal{O}(K_H)$.
  3. Sort alternatives in ascending order: $\mathcal{O}(K_H \log K_H)$.
  - Total selection time:
    $$T_{\text{hosp}} = \mathbf{\mathcal{O}((V + E) \log V + K_H \log K_H)}$$
  - For Metropolis Central ($K_H = 3$):
    $$K_H \log_2 K_H = 3 \times 1.58 \approx 5 \text{ comparisons. Total time dominated by single Dijkstra run } (< 0.1\text{ ms).}$$

