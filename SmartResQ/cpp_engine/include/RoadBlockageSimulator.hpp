#pragma once

#include "Graph.hpp"
#include "GraphAlgorithms.hpp"
#include <string>
#include <vector>
#include <iostream>

namespace smartresq {

/**
 * @brief Complete before-and-after comparison report for a road blockage event.
 */
struct BlockageRerouteResult {
    bool simulation_success{false};
    std::string blocked_road_id;
    std::string origin_id;
    std::string destination_id;
    std::string algorithm_used{"Dijkstra"};

    RouteResult original_route;
    RouteResult updated_route;

    bool detour_found{false};
    bool no_alternate_route{false};
    double distance_difference_km{0.0}; // updated - original (> 0 indicates extra distance)
    double percentage_increase{0.0};    // % distance increase
    int hop_difference{0};              // updated_hops - original_hops

    std::string message;

    /**
     * @brief Formats and displays original vs updated routes, trajectory diffs, and distance deltas.
     */
    void print(std::ostream& os = std::cout) const;
};

/**
 * @brief Simulates dynamic road blockages, edge exclusions, and shortest route recalculations.
 * 
 * TIME & SPACE COMPLEXITY:
 * - simulateBlockageAndReroute:
 *   Time Complexity:  O((|V| + |E|) log |V|)
 *   Runs baseline Dijkstra/BFS, performs O(|E|) edge closure, and runs recalculated Dijkstra/BFS.
 *   Space Complexity: O(|V|)
 * 
 * - reopenRoad / resetAllBlockages:
 *   Time Complexity:  O(|E|)
 *   Space Complexity: O(1)
 */
class RoadBlockageSimulator {
public:
    explicit RoadBlockageSimulator(CityGraph& graph);

    /**
     * @brief Blocks an existing road, recalculates the route using Dijkstra (or BFS),
     *        and produces a before-and-after comparison with distance difference.
     * @param road_id The road identifier to block (e.g. "R01").
     * @param origin_id Route start location ID.
     * @param destination_id Route target location ID.
     * @param use_bfs If true, uses BFS hop-minimal solver instead of Dijkstra distance-minimal solver.
     * @return BlockageRerouteResult containing complete comparison metrics.
     */
    BlockageRerouteResult simulateBlockageAndReroute(const std::string& road_id,
                                                    const std::string& origin_id,
                                                    const std::string& destination_id,
                                                    bool use_bfs = false);

    /**
     * @brief Blocks road connecting two specific location vertices and recalculates route.
     */
    BlockageRerouteResult simulateBlockageByEndpoints(const std::string& source_id,
                                                      const std::string& destination_id,
                                                      const std::string& origin_id,
                                                      const std::string& target_id,
                                                      bool use_bfs = false);

    /**
     * @brief Reopens a specific road in the graph.
     * @param road_id Identifier of the road to reopen.
     * @return true if reopened successfully; false otherwise.
     */
    bool reopenRoad(const std::string& road_id);

    /**
     * @brief Reopens all blocked roads, restoring the original unblocked graph state.
     */
    void resetAllBlockages();

    /**
     * @brief Returns a list of all currently blocked road IDs.
     */
    std::vector<std::string> getBlockedRoads() const;

    /**
     * @brief Checks if a specific road is currently blocked in the graph.
     */
    bool isRoadBlocked(const std::string& road_id) const;

private:
    CityGraph& graph_;
};

} // namespace smartresq

