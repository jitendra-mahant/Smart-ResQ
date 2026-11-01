#include "GraphAlgorithms.hpp"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iomanip>

namespace smartresq {

// ============================================================================
// RouteResult & TraversalResult Output Formatting
// ============================================================================

void RouteResult::print(std::ostream& os) const {
    os << "--------------------------------------------------------------------\n";
    os << "  Algorithm Used:      " << algorithm_used << "\n";
    os << "  Source Location:     " << source_id << "\n";
    os << "  Destination:         " << destination_id << "\n";
    os << "  Status:              " << (found ? "ROUTE FOUND [SUCCESS]" : "NO PATH [UNREACHABLE]") << "\n";

    if (found) {
        os << "  Total Road Distance: " << std::fixed << std::setprecision(2) << total_distance_km << " km\n";
        os << "  Hop Count (Roads):   " << hop_count << " segments\n";
        os << "  Route Trajectory:    ";
        for (size_t i = 0; i < path_nodes.size(); ++i) {
            os << path_nodes[i] << (i + 1 < path_nodes.size() ? " -> " : "");
        }
        os << "\n  Roads Traversed:     ";
        for (size_t i = 0; i < road_ids.size(); ++i) {
            os << road_ids[i] << (i + 1 < road_ids.size() ? " -> " : "");
        }
        os << "\n";
    }
    os << "--------------------------------------------------------------------\n";
}

void TraversalResult::print(std::ostream& os) const {
    os << "--------------------------------------------------------------------\n";
    os << "  Algorithm Used:      " << algorithm_used << "\n";
    os << "  Start Location:      " << start_id << "\n";
    os << "  Reachable Locations: " << reachable_count << " nodes\n";
    os << "  Traversal Order:     ";
    for (size_t i = 0; i < visited_nodes.size(); ++i) {
        os << visited_nodes[i] << (i + 1 < visited_nodes.size() ? " -> " : "");
    }
    os << "\n--------------------------------------------------------------------\n";
}

// ============================================================================
// 1. Dijkstra's Algorithm (Min-Heap Priority Queue)
// ============================================================================

RouteResult GraphAlgorithms::dijkstra(const CityGraph& graph,
                                     const std::string& start_node,
                                     const std::string& target_node) {
    RouteResult result;
    result.algorithm_used = "Dijkstra (Min-Heap Priority Queue)";
    result.source_id = start_node;
    result.destination_id = target_node;

    // Boundary check: endpoints must exist
    if (!graph.hasLocation(start_node) || !graph.hasLocation(target_node)) {
        return result;
    }

    // Trivial case: source is destination
    if (start_node == target_node) {
        result.found = true;
        result.total_distance_km = 0.0;
        result.hop_count = 0;
        result.path_nodes = {start_node};
        return result;
    }

    // Min-heap storing pair: (tentative_distance, node_id)
    using HeapElement = std::pair<double, std::string>;
    std::priority_queue<HeapElement, std::vector<HeapElement>, std::greater<HeapElement>> min_heap;

    // Distances map initialized to infinity
    std::unordered_map<std::string, double> distances;
    // Maps child_node -> pair(parent_node, road_id)
    std::unordered_map<std::string, std::pair<std::string, std::string>> parent;

    for (const auto& [id, _] : graph.getAllLocations()) {
        distances[id] = std::numeric_limits<double>::infinity();
    }

    distances[start_node] = 0.0;
    min_heap.push({0.0, start_node});

    while (!min_heap.empty()) {
        auto [current_dist, u] = min_heap.top();
        min_heap.pop();

        // Target reached: early termination
        if (u == target_node) {
            break;
        }

        // Skip stale entries
        if (current_dist > distances[u]) {
            continue;
        }

        // Iterate through active (non-blocked) outgoing roads
        for (const auto& edge : graph.getOutgoingRoads(u, false)) {
            if (edge.is_blocked) continue;

            const std::string& v = edge.destination_id;
            double weight = edge.distance_km;

            // Relaxation step
            if (distances[u] + weight < distances[v]) {
                distances[v] = distances[u] + weight;
                parent[v] = {u, edge.road_id};
                min_heap.push({distances[v], v});
            }
        }
    }

    // If destination is unreachable
    if (distances[target_node] == std::numeric_limits<double>::infinity()) {
        return result;
    }

    // Reconstruct path backwards
    result.found = true;
    result.total_distance_km = distances[target_node];

    std::vector<std::string> nodes_rev;
    std::vector<std::string> roads_rev;
    std::string curr = target_node;

    nodes_rev.push_back(curr);
    while (curr != start_node) {
        auto it = parent.find(curr);
        if (it == parent.end()) break;
        roads_rev.push_back(it->second.second);
        curr = it->second.first;
        nodes_rev.push_back(curr);
    }

    std::reverse(nodes_rev.begin(), nodes_rev.end());
    std::reverse(roads_rev.begin(), roads_rev.end());

    result.path_nodes = std::move(nodes_rev);
    result.road_ids = std::move(roads_rev);
    result.hop_count = result.road_ids.size();

    return result;
}

// ============================================================================
// 2. Breadth-First Search (BFS - Minimum Hop Route)
// ============================================================================

RouteResult GraphAlgorithms::bfs(const CityGraph& graph,
                                 const std::string& start_node,
                                 const std::string& target_node) {
    RouteResult result;
    result.algorithm_used = "BFS (Minimum Hops Queue)";
    result.source_id = start_node;
    result.destination_id = target_node;

    if (!graph.hasLocation(start_node) || !graph.hasLocation(target_node)) {
        return result;
    }

    if (start_node == target_node) {
        result.found = true;
        result.total_distance_km = 0.0;
        result.hop_count = 0;
        result.path_nodes = {start_node};
        return result;
    }

    std::queue<std::string> q;
    std::unordered_set<std::string> visited;
    // Maps child_node -> tuple(parent_node, road_id, road_distance_km)
    struct ParentEdge {
        std::string parent_node;
        std::string road_id;
        double distance_km{0.0};
    };
    std::unordered_map<std::string, ParentEdge> parent;

    visited.insert(start_node);
    q.push(start_node);

    bool reached = false;

    while (!q.empty()) {
        std::string u = q.front();
        q.pop();

        if (u == target_node) {
            reached = true;
            break;
        }

        for (const auto& edge : graph.getOutgoingRoads(u, false)) {
            if (edge.is_blocked) continue;

            const std::string& v = edge.destination_id;
            if (visited.find(v) == visited.end()) {
                visited.insert(v);
                parent[v] = {u, edge.road_id, edge.distance_km};
                q.push(v);

                if (v == target_node) {
                    reached = true;
                    break;
                }
            }
        }
        if (reached) break;
    }

    if (!reached) {
        return result;
    }

    // Reconstruct BFS path
    result.found = true;
    double cumulative_distance = 0.0;

    std::vector<std::string> nodes_rev;
    std::vector<std::string> roads_rev;
    std::string curr = target_node;

    nodes_rev.push_back(curr);
    while (curr != start_node) {
        auto it = parent.find(curr);
        if (it == parent.end()) break;
        roads_rev.push_back(it->second.road_id);
        cumulative_distance += it->second.distance_km;
        curr = it->second.parent_node;
        nodes_rev.push_back(curr);
    }

    std::reverse(nodes_rev.begin(), nodes_rev.end());
    std::reverse(roads_rev.begin(), roads_rev.end());

    result.total_distance_km = cumulative_distance;
    result.path_nodes = std::move(nodes_rev);
    result.road_ids = std::move(roads_rev);
    result.hop_count = result.road_ids.size();

    return result;
}

// ============================================================================
// 3. Depth-First Search (DFS - Traversal Order)
// ============================================================================

void GraphAlgorithms::dfsRecursive(const CityGraph& graph,
                                   const std::string& current,
                                   std::unordered_set<std::string>& visited,
                                   std::vector<std::string>& order) {
    visited.insert(current);
    order.push_back(current);

    for (const auto& edge : graph.getOutgoingRoads(current, false)) {
        if (edge.is_blocked) continue;

        if (visited.find(edge.destination_id) == visited.end()) {
            dfsRecursive(graph, edge.destination_id, visited, order);
        }
    }
}

TraversalResult GraphAlgorithms::dfs(const CityGraph& graph,
                                     const std::string& start_node) {
    TraversalResult result;
    result.start_id = start_node;
    result.algorithm_used = "DFS (Depth-First Search)";

    if (!graph.hasLocation(start_node)) {
        return result;
    }

    std::unordered_set<std::string> visited;
    std::vector<std::string> order;

    dfsRecursive(graph, start_node, visited, order);

    result.visited_nodes = std::move(order);
    result.reachable_count = result.visited_nodes.size();

    return result;
}

// ============================================================================
// 4. Candidate Nearest Target Selection
// ============================================================================

NearestCandidateResult GraphAlgorithms::findNearestCandidate(const CityGraph& graph,
                                                             const std::string& origin_node,
                                                             const std::vector<std::string>& candidates) {
    NearestCandidateResult best;
    best.found = false;
    best.distance_km = std::numeric_limits<double>::infinity();

    for (const auto& candidate : candidates) {
        auto route = dijkstra(graph, origin_node, candidate);
        if (route.found && route.total_distance_km < best.distance_km) {
            best.found = true;
            best.best_candidate_id = candidate;
            best.distance_km = route.total_distance_km;
            best.route = std::move(route);
        }
    }

    return best;
}

} // namespace smartresq

