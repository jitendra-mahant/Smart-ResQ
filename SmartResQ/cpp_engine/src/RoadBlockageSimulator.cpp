#include "RoadBlockageSimulator.hpp"
#include <iomanip>
#include <cmath>

namespace smartresq {

// ============================================================================
// Output Formatter
// ============================================================================

void BlockageRerouteResult::print(std::ostream& os) const {
    os << "====================================================================\n";
    os << "  ROAD BLOCKAGE & DYNAMIC DETOUR REROUTING REPORT\n";
    os << "====================================================================\n";
    os << "  Blocked Road ID:     [" << blocked_road_id << "]\n";
    os << "  Route Query:         [" << origin_id << "] -> [" << destination_id << "]\n";
    os << "  Algorithm Applied:   " << algorithm_used << "\n";

    if (!simulation_success) {
        os << "  Simulation Status:   FAILED (" << message << ")\n";
        os << "====================================================================\n";
        return;
    }

    if (no_alternate_route) {
        os << "  Simulation Status:   NO ALTERNATE ROUTE EXISTS [DESTINATION UNREACHABLE]\n";
        os << "  Alert Message:       " << message << "\n";
        os << "\n  [1] ORIGINAL ROUTE (Before Closure):\n";
        os << "      - Total Distance: " << std::fixed << std::setprecision(2)
           << original_route.total_distance_km << " km (" << original_route.hop_count << " segments)\n";
        os << "      - Trajectory:     ";
        for (size_t i = 0; i < original_route.path_nodes.size(); ++i) {
            os << original_route.path_nodes[i] << (i + 1 < original_route.path_nodes.size() ? " -> " : "");
        }
        os << "\n\n  [2] UPDATED DETOUR ROUTE (Under Closure):\n";
        os << "      - Status:         SEVERED (0 paths available)\n";
        os << "====================================================================\n";
        return;
    }

    os << "  Simulation Status:   DETOUR FOUND [GRAPH SHORTCUT RECALCULATED]\n";
    os << "  Message:             " << message << "\n\n";

    // Original Route
    os << "  [1] ORIGINAL ROUTE (Before Closure):\n";
    os << "      - Road Distance:  " << std::fixed << std::setprecision(2)
       << original_route.total_distance_km << " km (" << original_route.hop_count << " road segments)\n";
    os << "      - Trajectory:     ";
    for (size_t i = 0; i < original_route.path_nodes.size(); ++i) {
        os << original_route.path_nodes[i] << (i + 1 < original_route.path_nodes.size() ? " -> " : "");
    }
    os << "\n      - Road Segments:  ";
    for (size_t i = 0; i < original_route.road_ids.size(); ++i) {
        os << original_route.road_ids[i] << (i + 1 < original_route.road_ids.size() ? " -> " : "");
    }
    os << "\n\n";

    // Updated Detour Route
    os << "  [2] UPDATED DETOUR ROUTE (Under Closure):\n";
    os << "      - Road Distance:  " << std::fixed << std::setprecision(2)
       << updated_route.total_distance_km << " km (" << updated_route.hop_count << " road segments)\n";
    os << "      - Detour Path:    ";
    for (size_t i = 0; i < updated_route.path_nodes.size(); ++i) {
        os << updated_route.path_nodes[i] << (i + 1 < updated_route.path_nodes.size() ? " -> " : "");
    }
    os << "\n      - Detour Roads:   ";
    for (size_t i = 0; i < updated_route.road_ids.size(); ++i) {
        os << updated_route.road_ids[i] << (i + 1 < updated_route.road_ids.size() ? " -> " : "");
    }
    os << "\n\n";

    // Distance Impact Metrics
    os << "  [3] IMPACT COMPARISON:\n";
    os << "      - Distance Delta: " << (distance_difference_km >= 0 ? "+" : "")
       << std::fixed << std::setprecision(2) << distance_difference_km << " km ("
       << (percentage_increase >= 0 ? "+" : "") << std::fixed << std::setprecision(1) << percentage_increase << "%)\n";
    os << "      - Hop Count Delta:" << (hop_difference >= 0 ? "+" : "")
       << hop_difference << " road segments\n";
    os << "====================================================================\n";
}

// ============================================================================
// Constructor & Methods
// ============================================================================

RoadBlockageSimulator::RoadBlockageSimulator(CityGraph& graph)
    : graph_(graph) {}

BlockageRerouteResult RoadBlockageSimulator::simulateBlockageAndReroute(const std::string& road_id,
                                                                        const std::string& origin_id,
                                                                        const std::string& destination_id,
                                                                        bool use_bfs) {
    BlockageRerouteResult result;
    result.blocked_road_id = road_id;
    result.origin_id = origin_id;
    result.destination_id = destination_id;
    result.algorithm_used = use_bfs ? "BFS (Hop-Minimal)" : "Dijkstra (Distance-Minimal)";

    // 1. Validation checks
    if (!graph_.hasRoad(road_id)) {
        result.message = "Road ID [" + road_id + "] not found in city graph.";
        return result;
    }
    if (!graph_.hasLocation(origin_id) || !graph_.hasLocation(destination_id)) {
        result.message = "Origin or Destination location not found in city graph.";
        return result;
    }

    // 2. Compute Original Route before applying closure
    result.original_route = use_bfs ? GraphAlgorithms::bfs(graph_, origin_id, destination_id)
                                    : GraphAlgorithms::dijkstra(graph_, origin_id, destination_id);

    // 3. Modify actual graph edge availability: Block the road
    bool blocked = graph_.blockRoad(road_id);
    if (!blocked) {
        result.message = "Failed to block road [" + road_id + "].";
        return result;
    }

    // 4. Recalculate route using the modified graph
    result.updated_route = use_bfs ? GraphAlgorithms::bfs(graph_, origin_id, destination_id)
                                   : GraphAlgorithms::dijkstra(graph_, origin_id, destination_id);

    result.simulation_success = true;

    // 5. Check if alternate route exists
    if (!result.updated_route.found) {
        result.detour_found = false;
        result.no_alternate_route = true;
        result.message = "Road [" + road_id + "] blocked. No alternate route exists (Destination unreachable).";
    } else {
        result.detour_found = true;
        result.no_alternate_route = false;
        result.distance_difference_km = result.updated_route.total_distance_km -
                                        result.original_route.total_distance_km;

        if (result.original_route.total_distance_km > 0.0) {
            result.percentage_increase = (result.distance_difference_km / result.original_route.total_distance_km) * 100.0;
        } else {
            result.percentage_increase = 0.0;
        }

        result.hop_difference = static_cast<int>(result.updated_route.hop_count) -
                                static_cast<int>(result.original_route.hop_count);

        result.message = "Road [" + road_id + "] blocked. Detour route successfully recalculated.";
    }

    return result;
}

BlockageRerouteResult RoadBlockageSimulator::simulateBlockageByEndpoints(const std::string& source_id,
                                                                         const std::string& destination_id,
                                                                         const std::string& origin_id,
                                                                         const std::string& target_id,
                                                                         bool use_bfs) {
    BlockageRerouteResult result;
    result.blocked_road_id = source_id + "<->" + destination_id;
    result.origin_id = origin_id;
    result.destination_id = target_id;
    result.algorithm_used = use_bfs ? "BFS (Hop-Minimal)" : "Dijkstra (Distance-Minimal)";

    if (!graph_.hasLocation(source_id) || !graph_.hasLocation(destination_id) ||
        !graph_.hasLocation(origin_id) || !graph_.hasLocation(target_id)) {
        result.message = "Specified vertices do not exist in graph.";
        return result;
    }

    // Baseline route
    result.original_route = use_bfs ? GraphAlgorithms::bfs(graph_, origin_id, target_id)
                                    : GraphAlgorithms::dijkstra(graph_, origin_id, target_id);

    // Block endpoints
    bool blocked = graph_.blockRoad(source_id, destination_id);
    if (!blocked) {
        result.message = "No direct road between [" + source_id + "] and [" + destination_id + "] to block.";
        return result;
    }

    // Recalculate route
    result.updated_route = use_bfs ? GraphAlgorithms::bfs(graph_, origin_id, target_id)
                                   : GraphAlgorithms::dijkstra(graph_, origin_id, target_id);

    result.simulation_success = true;

    if (!result.updated_route.found) {
        result.detour_found = false;
        result.no_alternate_route = true;
        result.message = "No alternate route exists under this road closure.";
    } else {
        result.detour_found = true;
        result.no_alternate_route = false;
        result.distance_difference_km = result.updated_route.total_distance_km -
                                        result.original_route.total_distance_km;

        if (result.original_route.total_distance_km > 0.0) {
            result.percentage_increase = (result.distance_difference_km / result.original_route.total_distance_km) * 100.0;
        }

        result.hop_difference = static_cast<int>(result.updated_route.hop_count) -
                                static_cast<int>(result.original_route.hop_count);
        result.message = "Detour recalculated successfully.";
    }

    return result;
}

bool RoadBlockageSimulator::reopenRoad(const std::string& road_id) {
    return graph_.reopenRoad(road_id);
}

void RoadBlockageSimulator::resetAllBlockages() {
    graph_.reopenAllRoads();
}

std::vector<std::string> RoadBlockageSimulator::getBlockedRoads() const {
    return graph_.getBlockedRoads();
}

bool RoadBlockageSimulator::isRoadBlocked(const std::string& road_id) const {
    return graph_.isRoadBlocked(road_id);
}

} // namespace smartresq

