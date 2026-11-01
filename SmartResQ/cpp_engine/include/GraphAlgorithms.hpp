#pragma once

#include "Graph.hpp"
#include <string>
#include <vector>
#include <limits>
#include <iostream>

namespace smartresq {

/**
 * @brief Represents the route optimization result returned by graph pathfinding algorithms.
 */
struct RouteResult {
    bool found{false};
    std::string algorithm_used{"Unknown"};
    std::string source_id;
    std::string destination_id;
    double total_distance_km{std::numeric_limits<double>::infinity()};
    size_t hop_count{0};
    std::vector<std::string> path_nodes;
    std::vector<std::string> road_ids;

    /**
     * @brief Displays the route, distance, and algorithm details in formatted output.
     */
    void print(std::ostream& os = std::cout) const;
};

/**
 * @brief Represents selection of the closest target from a set of candidates.
 */
struct NearestCandidateResult {
    bool found{false};
    std::string best_candidate_id;
    double distance_km{std::numeric_limits<double>::infinity()};
    RouteResult route;
};

/**
 * @brief Represents the result of a depth-first or complete graph traversal.
 */
struct TraversalResult {
    std::string start_id;
    std::string algorithm_used{"DFS (Depth-First Search)"};
    std::vector<std::string> visited_nodes;
    size_t reachable_count{0};

    /**
     * @brief Displays the traversal order and reachable vertices.
     */
    void print(std::ostream& os = std::cout) const;
};

/**
 * @brief Core Graph Algorithms Engine implementing Dijkstra, BFS, and DFS.
 * 
 * ============================================================================
 * TIME & SPACE COMPLEXITY ANALYSIS:
 * ============================================================================
 * 1. Dijkstra's Algorithm (Min-Heap Priority Queue):
 *    - Time Complexity:  O((|V| + |E|) log |V|)
 *      Each vertex is inserted into the min-heap at most once per relaxation.
 *      Extract-min takes O(log |V|), and each edge relaxation takes O(log |V|).
 *    - Space Complexity: O(|V|)
 *      Maintains distance map (O(|V|)), parent map (O(|V|)), and heap (O(|V|)).
 * 
 * 2. Breadth-First Search (BFS - Minimum Hop Route):
 *    - Time Complexity:  O(|V| + |E|)
 *      Standard queue FIFO traversal. Each vertex is visited once, and each
 *      incident edge is examined once. Finds the route with minimal road segments.
 *    - Space Complexity: O(|V|)
 *      Queue holds at most O(|V|) vertices; visited set and parent map hold O(|V|).
 * 
 * 3. Depth-First Search (DFS - Full Reachability Traversal):
 *    - Time Complexity:  O(|V| + |E|)
 *      Traverses deep along each branch before backtracking. Visits every
 *      reachable vertex and edge in the connected component once.
 *    - Space Complexity: O(|V|)
 *      Recursion call stack / visited set holds at most O(|V|) vertices.
 * ============================================================================
 */
class GraphAlgorithms {
public:
    /**
     * @brief Computes the minimum-distance route between two locations using Dijkstra's algorithm.
     * @param graph The city road network.
     * @param start_node Source location ID.
     * @param target_node Destination location ID.
     * @return RouteResult with minimum distance, complete path nodes, and road IDs.
     */
    static RouteResult dijkstra(const CityGraph& graph,
                                const std::string& start_node,
                                const std::string& target_node);

    /**
     * @brief Computes the route with the minimum number of road segments (hops) using BFS.
     * @param graph The city road network.
     * @param start_node Source location ID.
     * @param target_node Destination location ID.
     * @return RouteResult with minimum hops, cumulative distance, path nodes, and road IDs.
     */
    static RouteResult bfs(const CityGraph& graph,
                           const std::string& start_node,
                           const std::string& target_node);

    /**
     * @brief Traverses all reachable locations starting from start_node using DFS.
     * @param graph The city road network.
     * @param start_node Starting location ID.
     * @return TraversalResult containing traversal order and total reachable vertex count.
     */
    static TraversalResult dfs(const CityGraph& graph,
                               const std::string& start_node);

    /**
     * @brief Given an origin location and a list of candidate targets (e.g. ambulances or hospitals),
     *        finds the candidate with the shortest road distance via Dijkstra.
     */
    static NearestCandidateResult findNearestCandidate(const CityGraph& graph,
                                                        const std::string& origin_node,
                                                        const std::vector<std::string>& candidates);

private:
    static void dfsRecursive(const CityGraph& graph,
                             const std::string& current,
                             std::unordered_set<std::string>& visited,
                             std::vector<std::string>& order);
};

} // namespace smartresq

