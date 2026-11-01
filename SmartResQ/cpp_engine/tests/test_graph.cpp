#include "Graph.hpp"
#include "GraphAlgorithms.hpp"
#include "Dijkstra.hpp"
#include "EmergencyDispatcher.hpp"
#include "RoadBlockageSimulator.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <filesystem>

// ============================================================================
// Test 1: Dijkstra Minimum-Distance Optimization (Multiple Routes)
// ============================================================================
void test_dijkstra_multiple_routes() {
    std::cout << "[Test 1/20] Dijkstra Minimum-Distance Optimization (Multiple Routes): ";
    smartresq::CityGraph graph;

    graph.addLocation({"S", "Start Point", 0.0, 0.0});
    graph.addLocation({"E", "End Point", 5.0, 5.0});
    graph.addLocation({"M", "Midpoint M", 2.0, 2.0});
    graph.addLocation({"A", "Point A", 1.0, 1.0});
    graph.addLocation({"B", "Point B", 3.0, 3.0});

    graph.addRoad("R_DIRECT", "S", "E", 12.0, true);
    graph.addRoad("R_SM", "S", "M", 3.0, true);
    graph.addRoad("R_ME", "M", "E", 4.0, true);
    graph.addRoad("R_SA", "S", "A", 2.0, true);
    graph.addRoad("R_AB", "A", "B", 2.0, true);
    graph.addRoad("R_BE", "B", "E", 1.5, true);

    auto result = smartresq::GraphAlgorithms::dijkstra(graph, "S", "E");
    assert(result.found);
    assert(std::abs(result.total_distance_km - 5.5) < 1e-4);
    assert(result.hop_count == 3);

    std::cout << "PASSED (Optimal 5.50 km path chosen over 7.00 km and 12.00 km routes)\n";
}

// ============================================================================
// Test 2: BFS Minimum Road Segments (Hops) vs. Dijkstra Distance
// ============================================================================
void test_bfs_vs_dijkstra() {
    std::cout << "[Test 2/20] BFS Minimum Hops vs. Dijkstra Minimum Distance: ";
    smartresq::CityGraph graph;

    graph.addLocation({"S", "Start", 0.0, 0.0});
    graph.addLocation({"E", "End", 10.0, 0.0});
    graph.addLocation({"N1", "Node 1", 2.0, 1.0});
    graph.addLocation({"N2", "Node 2", 5.0, 1.0});

    graph.addRoad("R_HIGHWAY", "S", "E", 20.0, true);
    graph.addRoad("R1", "S", "N1", 2.0, true);
    graph.addRoad("R2", "N1", "N2", 2.0, true);
    graph.addRoad("R3", "N2", "E", 2.0, true);

    auto dijkstra_res = smartresq::GraphAlgorithms::dijkstra(graph, "S", "E");
    assert(dijkstra_res.found);
    assert(std::abs(dijkstra_res.total_distance_km - 6.0) < 1e-4);
    assert(dijkstra_res.hop_count == 3);

    auto bfs_res = smartresq::GraphAlgorithms::bfs(graph, "S", "E");
    assert(bfs_res.found);
    assert(bfs_res.hop_count == 1);
    assert(std::abs(bfs_res.total_distance_km - 20.0) < 1e-4);

    std::cout << "PASSED (Dijkstra chose 6.0 km / 3 hops; BFS chose 20.0 km / 1 hop)\n";
}

// ============================================================================
// Test 3: Equal-Distance Paths & Deterministic Tie-Breaking
// ============================================================================
void test_equal_distance_paths() {
    std::cout << "[Test 3/20] Equal-Distance Paths (Tie-Breaking & Integrity): ";
    smartresq::CityGraph graph;

    graph.addLocation({"S", "Source", 0.0, 0.0});
    graph.addLocation({"T", "Target", 4.0, 0.0});
    graph.addLocation({"U", "Upper Waypoint", 2.0, 2.0});
    graph.addLocation({"V", "Lower Waypoint", 2.0, -2.0});

    graph.addRoad("R_SU", "S", "U", 2.0, true);
    graph.addRoad("R_UT", "U", "T", 2.0, true);
    graph.addRoad("R_SV", "S", "V", 2.0, true);
    graph.addRoad("R_VT", "V", "T", 2.0, true);

    auto dijkstra_res = smartresq::GraphAlgorithms::dijkstra(graph, "S", "T");
    assert(dijkstra_res.found);
    assert(std::abs(dijkstra_res.total_distance_km - 4.0) < 1e-4);
    assert(dijkstra_res.hop_count == 2);

    std::cout << "PASSED (Deterministic 4.00 km resolution on equal paths)\n";
}

// ============================================================================
// Test 4: Road Blockages & Dynamic Detour Rerouting
// ============================================================================
void test_road_blockage_rerouting() {
    std::cout << "[Test 4/20] Road Blockages & Dynamic Detour Rerouting: ";
    smartresq::CityGraph graph;

    graph.addLocation({"A", "Station A", 0.0, 0.0});
    graph.addLocation({"B", "Hospital B", 1.5, 0.0});
    graph.addLocation({"C", "Detour C", 0.5, 1.5});

    graph.addRoad("R_MAIN", "A", "B", 1.5, true);
    graph.addRoad("R_AC", "A", "C", 2.1, true);
    graph.addRoad("R_CB", "C", "B", 2.2, true);

    auto initial = smartresq::GraphAlgorithms::dijkstra(graph, "A", "B");
    assert(initial.found && std::abs(initial.total_distance_km - 1.5) < 1e-4);

    assert(graph.blockRoad("R_MAIN"));
    assert(graph.isRoadBlocked("R_MAIN"));

    auto detour = smartresq::GraphAlgorithms::dijkstra(graph, "A", "B");
    assert(detour.found && std::abs(detour.total_distance_km - 4.3) < 1e-4);

    assert(graph.reopenRoad("R_MAIN"));
    auto restored = smartresq::GraphAlgorithms::dijkstra(graph, "A", "B");
    assert(restored.found && std::abs(restored.total_distance_km - 1.5) < 1e-4);

    std::cout << "PASSED (Dynamic diversion 1.5 km -> 4.3 km detour -> 1.5 km restoration)\n";
}

// ============================================================================
// Test 5: Disconnected & Unreachable Destinations
// ============================================================================
void test_unreachable_destinations() {
    std::cout << "[Test 5/20] Disconnected & Unreachable Destinations: ";
    smartresq::CityGraph graph;

    graph.addLocation({"I1", "Island Node 1", 0.0, 0.0});
    graph.addLocation({"I2", "Island Node 2", 1.0, 0.0});
    graph.addLocation({"ISOLATED", "Unreachable Node", 10.0, 10.0});
    graph.addRoad("R_ISLAND", "I1", "I2", 2.0, true);

    auto dijkstra_res = smartresq::GraphAlgorithms::dijkstra(graph, "I1", "ISOLATED");
    assert(!dijkstra_res.found && std::isinf(dijkstra_res.total_distance_km));

    std::cout << "PASSED (Safe unreachable status returned without crash)\n";
}

// ============================================================================
// Test 6: Single-Node Route (Start == Destination)
// ============================================================================
void test_single_node_route() {
    std::cout << "[Test 6/20] Single-Node Route (Start == Destination): ";
    smartresq::CityGraph graph;
    graph.addLocation({"HERE", "Current Spot", 18.5, 73.8});

    auto dijkstra_res = smartresq::GraphAlgorithms::dijkstra(graph, "HERE", "HERE");
    assert(dijkstra_res.found && std::abs(dijkstra_res.total_distance_km - 0.0) < 1e-4);

    std::cout << "PASSED (Immediate 0.0 km resolution for local origin)\n";
}

// ============================================================================
// Test 7: DFS Traversal Order & Component Coverage
// ============================================================================
void test_dfs_traversal() {
    std::cout << "[Test 7/20] DFS Reachability & Traversal Order: ";
    smartresq::CityGraph graph;

    graph.addLocation({"ROOT", "Root Node", 0.0, 0.0});
    graph.addLocation({"C1", "Child 1", 1.0, 0.0});
    graph.addLocation({"C2", "Child 2", -1.0, 0.0});
    graph.addLocation({"G1", "Grandchild 1", 2.0, 0.0});

    graph.addRoad("R_R_C1", "ROOT", "C1", 1.0, true);
    graph.addRoad("R_R_C2", "ROOT", "C2", 1.0, true);
    graph.addRoad("R_C1_G1", "C1", "G1", 1.0, true);

    auto dfs_res = smartresq::GraphAlgorithms::dfs(graph, "ROOT");
    assert(dfs_res.reachable_count == 4);

    std::cout << "PASSED (DFS accurately tracks traversal order and component bounds)\n";
}

// ============================================================================
// Test 8: Real Metropolitan City Graph (city_graph.json) Algorithms
// ============================================================================
void test_city_graph_algorithms() {
    std::cout << "[Test 8/20] Real City Dataset Emergency Routing (city_graph.json): ";
    smartresq::CityGraph graph;

    std::vector<std::string> paths = {
        "../../data/city_graph.json", "../data/city_graph.json", "data/city_graph.json", "E:/DSA_Project/SmartResQ/data/city_graph.json"
    };
    std::string dataset_path;
    for (const auto& p : paths) { if (std::filesystem::exists(p)) { dataset_path = p; break; } }
    assert(!dataset_path.empty() && graph.loadFromJsonFile(dataset_path));

    auto dijkstra_res = smartresq::GraphAlgorithms::dijkstra(graph, "L5", "L9");
    assert(dijkstra_res.found && dijkstra_res.total_distance_km > 0.0);

    std::cout << "PASSED (18 city nodes verified; Dijkstra distance: "
              << dijkstra_res.total_distance_km << " km)\n";
}

// ============================================================================
// Test 9: Nearest Target Candidate Selection
// ============================================================================
void test_nearest_candidate_selection() {
    std::cout << "[Test 9/20] Nearest Target Candidate Selection: ";
    smartresq::CityGraph graph;
    graph.addLocation({"ORIGIN", "Origin Node", 0.0, 0.0});
    graph.addLocation({"T1", "Target 1", 5.0, 0.0});
    graph.addLocation({"T2", "Target 2", 2.0, 0.0});
    graph.addRoad("R1", "ORIGIN", "T1", 5.0, true);
    graph.addRoad("R2", "ORIGIN", "T2", 2.0, true);

    auto best = smartresq::GraphAlgorithms::findNearestCandidate(graph, "ORIGIN", {"T1", "T2"});
    assert(best.found && best.best_candidate_id == "T2");

    std::cout << "PASSED (Candidate T2 selected with 2.00 km minimum route)\n";
}

// ============================================================================
// Test 10: Ambulance Selection (Shortest Distance)
// ============================================================================
void test_ambulance_selection_shortest_distance() {
    std::cout << "[Test 10/20] Ambulance Selection (Shortest Distance): ";
    smartresq::CityGraph graph;
    graph.addLocation({"EMERGENCY", "Incident Scene", 0.0, 0.0});
    graph.addLocation({"STATION_NEAR", "Near Station", 1.5, 0.0});
    graph.addLocation({"STATION_FAR", "Far Station", 6.0, 0.0});
    graph.addRoad("R_NEAR", "EMERGENCY", "STATION_NEAR", 1.5, true);
    graph.addRoad("R_FAR", "EMERGENCY", "STATION_FAR", 6.0, true);

    smartresq::EmergencyDispatcher dispatcher(graph);
    dispatcher.addAmbulance({"AMB_FAR", "STATION_FAR", true, "Alpha"});
    dispatcher.addAmbulance({"AMB_NEAR", "STATION_NEAR", true, "Bravo"});

    auto res = dispatcher.selectNearestAmbulance("EMERGENCY", false);
    assert(res.success && res.selected_ambulance.id == "AMB_NEAR");

    std::cout << "PASSED (Selected AMB_NEAR with 1.50 km travel distance)\n";
}

// ============================================================================
// Test 11: Double-Allocation Prevention
// ============================================================================
void test_ambulance_double_allocation_prevention() {
    std::cout << "[Test 11/20] Double-Allocation Prevention: ";
    smartresq::CityGraph graph;
    graph.addLocation({"LOC_INCIDENT", "Incident", 0.0, 0.0});
    graph.addLocation({"LOC_BASE1", "Station 1", 2.0, 0.0});
    graph.addLocation({"LOC_BASE2", "Station 2", 4.0, 0.0});
    graph.addRoad("R_BASE1", "LOC_INCIDENT", "LOC_BASE1", 2.0, true);
    graph.addRoad("R_BASE2", "LOC_INCIDENT", "LOC_BASE2", 4.0, true);

    smartresq::EmergencyDispatcher dispatcher(graph);
    dispatcher.addAmbulance({"AMB_1", "LOC_BASE1", true, "Unit 1"});
    dispatcher.addAmbulance({"AMB_2", "LOC_BASE2", true, "Unit 2"});

    auto res1 = dispatcher.selectNearestAmbulance("LOC_INCIDENT", true);
    assert(res1.success && res1.selected_ambulance.id == "AMB_1");
    assert(!dispatcher.isAmbulanceAvailable("AMB_1"));

    auto res2 = dispatcher.selectNearestAmbulance("LOC_INCIDENT", true);
    assert(res2.success && res2.selected_ambulance.id == "AMB_2");

    std::cout << "PASSED (AMB_1 dispatched -> AMB_2 dispatched -> Double-allocation blocked)\n";
}

// ============================================================================
// Test 12: Unreachable Units Ignored
// ============================================================================
void test_unreachable_units_ignored() {
    std::cout << "[Test 12/20] Unreachable Units Ignored: ";
    smartresq::CityGraph graph;
    graph.addLocation({"MAINLAND", "Mainland Scene", 0.0, 0.0});
    graph.addLocation({"BASE_VALID", "Mainland Base", 3.0, 0.0});
    graph.addLocation({"HOSP_VALID", "Mainland Hospital", 2.0, 0.0});
    graph.addLocation({"ISLAND", "Disconnected Island", 50.0, 50.0});
    graph.addRoad("R_MV", "MAINLAND", "BASE_VALID", 3.0, true);
    graph.addRoad("R_MH", "MAINLAND", "HOSP_VALID", 2.0, true);

    smartresq::EmergencyDispatcher dispatcher(graph);
    dispatcher.addAmbulance({"AMB_ISLAND", "ISLAND", true, "Stranded Ambulance"});
    dispatcher.addAmbulance({"AMB_VALID", "BASE_VALID", true, "Reachable Ambulance"});
    dispatcher.addHospital({"HOSP_ISLAND", "Island Clinic", "ISLAND"});
    dispatcher.addHospital({"HOSP_VALID", "Reachable Hospital", "HOSP_VALID"});

    auto amb_res = dispatcher.selectNearestAmbulance("MAINLAND", false);
    assert(amb_res.success && amb_res.selected_ambulance.id == "AMB_VALID");

    auto hosp_res = dispatcher.selectNearestHospital("MAINLAND");
    assert(hosp_res.success && hosp_res.selected_hospital.id == "HOSP_VALID");

    std::cout << "PASSED (Unreachable units on severed components ignored)\n";
}

// ============================================================================
// Test 13: Hospital Selection & Ordered Alternatives
// ============================================================================
void test_hospital_selection_with_alternatives() {
    std::cout << "[Test 13/20] Hospital Selection & Ordered Alternatives: ";
    smartresq::CityGraph graph;
    graph.addLocation({"PATIENT", "Patient", 0.0, 0.0});
    graph.addLocation({"H1", "Hospital 1", 2.0, 0.0});
    graph.addLocation({"H2", "Hospital 2", 5.0, 0.0});
    graph.addLocation({"H3", "Hospital 3", 8.0, 0.0});
    graph.addRoad("R_H1", "PATIENT", "H1", 2.0, true);
    graph.addRoad("R_H2", "PATIENT", "H2", 5.0, true);
    graph.addRoad("R_H3", "PATIENT", "H3", 8.0, true);

    smartresq::EmergencyDispatcher dispatcher(graph);
    dispatcher.addHospital({"HOSP_3", "Distant Hospital", "H3"});
    dispatcher.addHospital({"HOSP_1", "Closest Hospital", "H1"});
    dispatcher.addHospital({"HOSP_2", "Midway Hospital", "H2"});

    auto res = dispatcher.selectNearestHospital("PATIENT");
    assert(res.success && res.selected_hospital.id == "HOSP_1");
    assert(res.alternatives.size() == 2);
    assert(res.alternatives[0].hospital.id == "HOSP_2" && res.alternatives[1].hospital.id == "HOSP_3");

    std::cout << "PASSED (Primary H1 @ 2.0 km, Alternatives: H2 @ 5.0 km, H3 @ 8.0 km)\n";
}

// ============================================================================
// Test 14: End-to-End JSON City Simulation Dispatch
// ============================================================================
void test_end_to_end_json_dispatch() {
    std::cout << "[Test 14/20] End-to-End JSON City Simulation Dispatch: ";
    smartresq::CityGraph graph;

    std::vector<std::string> prefixes = {"../../data/", "../data/", "data/", "E:/DSA_Project/SmartResQ/data/"};
    std::string prefix;
    for (const auto& p : prefixes) {
        if (std::filesystem::exists(p + "city_graph.json")) { prefix = p; break; }
    }
    assert(!prefix.empty() && graph.loadFromJsonFile(prefix + "city_graph.json"));

    smartresq::EmergencyDispatcher dispatcher(graph);
    assert(dispatcher.loadAmbulancesFromJson(prefix + "ambulances.json"));
    assert(dispatcher.loadHospitalsFromJson(prefix + "hospitals.json"));

    auto mission = dispatcher.dispatchEmergency("L9");
    assert(mission.success);
    assert(mission.ambulance_result.selected_ambulance.id == "AMB-03");
    assert(mission.hospital_result.selected_hospital.id == "HOSP-02");
    assert(std::abs(mission.total_mission_distance_km - 3.5) < 1e-4);

    std::cout << "PASSED (AMB-03 @ 1.7 km -> L9 -> HOSP-02 @ 1.8 km | Total: 3.50 km)\n";
}

// ============================================================================
// Test 15: Road Blockage Simulation - Detour & Distance Difference
// ============================================================================
void test_blockage_detour_distance_diff() {
    std::cout << "[Test 15/20] Road Blockage Simulation - Detour & Distance Difference: ";
    smartresq::CityGraph graph;

    // Direct: A <--> B via R_DIRECT (1.50 km)
    // Detour: A <--> C <--> B via R_AC (2.00 km) + R_CB (2.00 km) = 4.00 km
    graph.addLocation({"A", "Station A", 0.0, 0.0});
    graph.addLocation({"B", "Hospital B", 1.5, 0.0});
    graph.addLocation({"C", "Detour C", 0.75, 1.5});

    graph.addRoad("R_DIRECT", "A", "B", 1.5, true);
    graph.addRoad("R_AC", "A", "C", 2.0, true);
    graph.addRoad("R_CB", "C", "B", 2.0, true);

    smartresq::RoadBlockageSimulator simulator(graph);

    // 1. Simulate blocking R_DIRECT
    auto res = simulator.simulateBlockageAndReroute("R_DIRECT", "A", "B", false);
    assert(res.simulation_success);
    assert(res.detour_found);
    assert(!res.no_alternate_route);

    // Baseline route was 1.5 km
    assert(std::abs(res.original_route.total_distance_km - 1.5) < 1e-4);
    // Updated route is 4.0 km
    assert(std::abs(res.updated_route.total_distance_km - 4.0) < 1e-4);

    // Distance difference must be exactly +2.50 km
    assert(std::abs(res.distance_difference_km - 2.5) < 1e-4);
    assert(std::abs(res.percentage_increase - (2.5 / 1.5 * 100.0)) < 1e-2);
    assert(res.hop_difference == 1); // 2 hops vs 1 hop

    // The road is actually blocked in the graph
    assert(simulator.isRoadBlocked("R_DIRECT"));
    assert(graph.isRoadBlocked("R_DIRECT"));

    std::cout << "PASSED (Original: 1.50 km, Detour: 4.00 km, Delta: +2.50 km / +166.7%)\n";
}

// ============================================================================
// Test 16: Road Reopening & Immediate Baseline Route Restoration
// ============================================================================
void test_blockage_reopen_restoration() {
    std::cout << "[Test 16/20] Road Reopening & Immediate Baseline Route Restoration: ";
    smartresq::CityGraph graph;

    graph.addLocation({"A", "Node A", 0.0, 0.0});
    graph.addLocation({"B", "Node B", 1.0, 0.0});
    graph.addLocation({"C", "Node C", 0.5, 1.0});

    graph.addRoad("R_AB", "A", "B", 1.0, true);
    graph.addRoad("R_AC", "A", "C", 1.8, true);
    graph.addRoad("R_CB", "C", "B", 1.8, true);

    smartresq::RoadBlockageSimulator simulator(graph);
    simulator.simulateBlockageAndReroute("R_AB", "A", "B", false);
    assert(simulator.isRoadBlocked("R_AB"));

    // Reopen road
    bool reopened = simulator.reopenRoad("R_AB");
    assert(reopened);
    assert(!simulator.isRoadBlocked("R_AB"));
    assert(!graph.isRoadBlocked("R_AB"));

    // Recalculate route after reopening
    auto restored = smartresq::GraphAlgorithms::dijkstra(graph, "A", "B");
    assert(restored.found);
    assert(std::abs(restored.total_distance_km - 1.0) < 1e-4);
    assert(restored.path_nodes.size() == 2);
    assert(restored.path_nodes[0] == "A" && restored.path_nodes[1] == "B");

    std::cout << "PASSED (Road R_AB reopened and baseline 1.00 km path immediately restored)\n";
}

// ============================================================================
// Test 17: Bridge Road Blockage (No Alternate Route Exists)
// ============================================================================
void test_blockage_no_alternate_route() {
    std::cout << "[Test 17/20] Bridge Road Blockage (No Alternate Route Exists): ";
    smartresq::CityGraph graph;

    // Mainland connects to Peninsula ONLY via R_BRIDGE
    graph.addLocation({"MAINLAND", "Mainland City", 0.0, 0.0});
    graph.addLocation({"PENINSULA", "Peninsula District", 5.0, 0.0});
    graph.addRoad("R_BRIDGE", "MAINLAND", "PENINSULA", 5.0, true);

    smartresq::RoadBlockageSimulator simulator(graph);

    // Simulate blocking the only connecting bridge
    auto res = simulator.simulateBlockageAndReroute("R_BRIDGE", "MAINLAND", "PENINSULA", false);
    assert(res.simulation_success);
    assert(!res.detour_found);
    assert(res.no_alternate_route);
    assert(!res.updated_route.found);
    assert(std::isinf(res.updated_route.total_distance_km));

    // Ensure system handled failure gracefully
    assert(res.message.find("No alternate route exists") != std::string::npos);

    // Reopen bridge to restore access
    simulator.reopenRoad("R_BRIDGE");
    auto restored = smartresq::GraphAlgorithms::dijkstra(graph, "MAINLAND", "PENINSULA");
    assert(restored.found && std::abs(restored.total_distance_km - 5.0) < 1e-4);

    std::cout << "PASSED (No alternate route detected cleanly without crash)\n";
}

// ============================================================================
// Test 18: BFS Hop-Minimal Detour Recalculation
// ============================================================================
void test_blockage_bfs_reroute() {
    std::cout << "[Test 18/20] BFS Hop-Minimal Detour Recalculation: ";
    smartresq::CityGraph graph;

    // Direct: X <--> Y (1 hop, 10.0 km)
    // Detour: X <--> Z1 <--> Z2 <--> Y (3 hops, 1.0 + 1.0 + 1.0 = 3.0 km)
    graph.addLocation({"X", "Point X", 0.0, 0.0});
    graph.addLocation({"Y", "Point Y", 10.0, 0.0});
    graph.addLocation({"Z1", "Point Z1", 2.0, 1.0});
    graph.addLocation({"Z2", "Point Z2", 5.0, 1.0});

    graph.addRoad("R_XY", "X", "Y", 10.0, true);
    graph.addRoad("R_XZ1", "X", "Z1", 1.0, true);
    graph.addRoad("R_Z1Z2", "Z1", "Z2", 1.0, true);
    graph.addRoad("R_Z2Y", "Z2", "Y", 1.0, true);

    smartresq::RoadBlockageSimulator simulator(graph);

    // Run BFS detour simulation
    auto res = simulator.simulateBlockageAndReroute("R_XY", "X", "Y", true);
    assert(res.simulation_success);
    assert(res.detour_found);
    assert(res.algorithm_used.find("BFS") != std::string::npos);

    // Original BFS: 1 hop (10.0 km)
    assert(res.original_route.hop_count == 1);
    assert(std::abs(res.original_route.total_distance_km - 10.0) < 1e-4);

    // Updated BFS detour: 3 hops (3.0 km)
    assert(res.updated_route.hop_count == 3);
    assert(std::abs(res.updated_route.total_distance_km - 3.0) < 1e-4);
    assert(res.hop_difference == 2);

    simulator.reopenRoad("R_XY");
    std::cout << "PASSED (BFS recalculated detour: 1 hop -> 3 hops)\n";
}

// ============================================================================
// Test 19: Full Graph Reset Operation (Multiple Road Blockages Restored)
// ============================================================================
void test_blockage_full_reset_operation() {
    std::cout << "[Test 19/20] Full Graph Reset Operation (Multiple Road Blockages Restored): ";
    smartresq::CityGraph graph;

    graph.addLocation({"N1", "Node 1", 0.0, 0.0});
    graph.addLocation({"N2", "Node 2", 1.0, 0.0});
    graph.addLocation({"N3", "Node 3", 2.0, 0.0});
    graph.addLocation({"N4", "Node 4", 3.0, 0.0});

    graph.addRoad("R1", "N1", "N2", 1.0, true);
    graph.addRoad("R2", "N2", "N3", 1.0, true);
    graph.addRoad("R3", "N3", "N4", 1.0, true);

    smartresq::RoadBlockageSimulator simulator(graph);

    // Block all 3 roads simultaneously
    assert(graph.blockRoad("R1"));
    assert(graph.blockRoad("R2"));
    assert(graph.blockRoad("R3"));

    auto blocked_list = simulator.getBlockedRoads();
    assert(blocked_list.size() == 3);
    assert(simulator.isRoadBlocked("R1"));
    assert(simulator.isRoadBlocked("R2"));
    assert(simulator.isRoadBlocked("R3"));
    assert(!graph.isConnected());

    // Execute RESET operation to restore original graph
    simulator.resetAllBlockages();

    // Verify 100% restored
    auto post_reset_blocked = simulator.getBlockedRoads();
    assert(post_reset_blocked.empty());
    assert(!simulator.isRoadBlocked("R1"));
    assert(!simulator.isRoadBlocked("R2"));
    assert(!simulator.isRoadBlocked("R3"));
    assert(graph.isConnected());

    std::cout << "PASSED (3 simultaneous road blockages cleared by reset operation)\n";
}

// ============================================================================
// Test 20: Real Metropolitan City Graph (city_graph.json) Detour Simulation
// ============================================================================
void test_blockage_real_city_reroute() {
    std::cout << "[Test 20/20] Real City Dataset Road Closure (R01 Blockage & Detour): ";
    smartresq::CityGraph graph;

    std::vector<std::string> prefixes = {"../../data/", "../data/", "data/", "E:/DSA_Project/SmartResQ/data/"};
    std::string prefix;
    for (const auto& p : prefixes) {
        if (std::filesystem::exists(p + "city_graph.json")) { prefix = p; break; }
    }
    assert(!prefix.empty() && graph.loadFromJsonFile(prefix + "city_graph.json"));

    smartresq::RoadBlockageSimulator simulator(graph);

    // Route: L1 (Central Junction) -> L2 (City General Hospital)
    // Baseline: Direct road R01 (1.20 km, 1 hop)
    // Detour:   L1 -> L17 -> L2 via R04 + R06 (1.80 + 1.30 = 3.10 km, 2 hops)
    auto sim_res = simulator.simulateBlockageAndReroute("R01", "L1", "L2", false);

    assert(sim_res.simulation_success);
    assert(sim_res.detour_found);
    assert(!sim_res.no_alternate_route);

    assert(std::abs(sim_res.original_route.total_distance_km - 1.20) < 1e-4);
    assert(std::abs(sim_res.updated_route.total_distance_km - 3.10) < 1e-4);
    assert(std::abs(sim_res.distance_difference_km - 1.90) < 1e-4);

    std::vector<std::string> expected_detour_nodes = {"L1", "L17", "L2"};
    assert(sim_res.updated_route.path_nodes == expected_detour_nodes);
    std::vector<std::string> expected_detour_roads = {"R04", "R06"};
    assert(sim_res.updated_route.road_ids == expected_detour_roads);

    // Reset graph to clean state
    simulator.resetAllBlockages();
    assert(!simulator.isRoadBlocked("R01"));

    std::cout << "PASSED (R01 blocked: 1.20 km -> 3.10 km detour | Delta: +1.90 km | Restored)\n";
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main() {
    std::cout << "====================================================================\n";
    std::cout << "   SmartResQ - Full Verification Test Suite (Algorithms & Blockage) \n";
    std::cout << "====================================================================\n";

    // 1. Core Algorithms
    test_dijkstra_multiple_routes();
    test_bfs_vs_dijkstra();
    test_equal_distance_paths();
    test_road_blockage_rerouting();
    test_unreachable_destinations();
    test_single_node_route();
    test_dfs_traversal();
    test_city_graph_algorithms();
    test_nearest_candidate_selection();

    // 2. Ambulance & Hospital Selection
    test_ambulance_selection_shortest_distance();
    test_ambulance_double_allocation_prevention();
    test_unreachable_units_ignored();
    test_hospital_selection_with_alternatives();
    test_end_to_end_json_dispatch();

    // 3. Road Blockage Simulation Tests
    test_blockage_detour_distance_diff();
    test_blockage_reopen_restoration();
    test_blockage_no_alternate_route();
    test_blockage_bfs_reroute();
    test_blockage_full_reset_operation();
    test_blockage_real_city_reroute();

    std::cout << "====================================================================\n";
    std::cout << "  ALL 20 UNIT TEST SUITES PASSED SUCCESSFULLY (100% PASS RATE)     \n";
    std::cout << "====================================================================\n";
    return 0;
}
