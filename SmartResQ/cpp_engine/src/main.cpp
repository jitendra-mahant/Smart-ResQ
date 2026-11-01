#include "Graph.hpp"
#include "GraphAlgorithms.hpp"
#include "EmergencyDispatcher.hpp"
#include "RoadBlockageSimulator.hpp"
#include "EngineApi.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <exception>

int main(int argc, char* argv[]) {
    // Locate data directory
    std::vector<std::string> prefixes = {
        "../../data",
        "../data",
        "data"
    };

    std::error_code path_error;
    const auto executable_path = std::filesystem::weakly_canonical(argv[0], path_error);
    if (!path_error) {
        const auto project_dir = executable_path.parent_path().parent_path().parent_path();
        prefixes.insert(prefixes.begin(), (project_dir / "data").string());
    }

    std::string data_dir;
    for (const auto& p : prefixes) {
        if (std::filesystem::exists(p + "/city_graph.json")) {
            data_dir = p;
            break;
        }
    }

    // JSON IPC Mode for Python Flask subprocess integration
    if (argc > 1 && (std::string(argv[1]) == "--json" || std::string(argv[1]) == "-j")) {
        std::string json_input;
        if (argc > 2) {
            json_input = argv[2];
        } else {
            std::stringstream buffer;
            buffer << std::cin.rdbuf();
            json_input = buffer.str();
        }

        try {
            smartresq::EngineApi api(data_dir.empty() ? "data" : data_dir);
            std::string response = api.processRequest(json_input);
            std::cout << response << std::endl;
        } catch (const std::exception& error) {
            std::cerr << "C++ engine request failed: " << error.what() << '\n';
            std::cout << "{\"status\":\"error\",\"error\":\"C++ engine could not process the request.\",\"code\":500}" << std::endl;
        }
        return 0;
    }

    // Interactive CLI Demonstration Mode
    std::cout << "====================================================================\n";
    std::cout << "   SmartResQ - Road Blockage & Dynamic Emergency Routing Simulation \n";
    std::cout << "   Third-Year Computer Engineering DSA Mini-Project                 \n";
    std::cout << "====================================================================\n\n";

    if (data_dir.empty()) {
        std::cerr << "Error: Unable to locate dataset folder.\n";
        return 1;
    }

    smartresq::CityGraph graph;
    std::cout << "[Step 1] Loading city network dataset from:\n         " << (data_dir + "/city_graph.json") << "\n";
    if (!graph.loadFromJsonFile(data_dir + "/city_graph.json")) {
        std::cerr << "Failed to parse city graph.\n";
        return 1;
    }
    std::cout << "--> Loaded " << graph.nodeCount() << " locations and "
              << graph.roadCount() << " directed road edges.\n\n";

    smartresq::EmergencyDispatcher dispatcher(graph);
    dispatcher.loadAmbulancesFromJson(data_dir + "/ambulances.json");
    dispatcher.loadHospitalsFromJson(data_dir + "/hospitals.json");

    smartresq::RoadBlockageSimulator blockage_sim(graph);

    // ========================================================================
    // SCENARIO 1: Road Blockage Simulation & Dynamic Detour Recalculation
    // ========================================================================
    std::cout << ">>> SCENARIO 1: Road Blockage Simulation & Detour Analysis\n";
    std::cout << "Route Query: Central Junction [L1] -> City General Hospital [L2]\n";

    std::string road_to_block = "R01";
    std::cout << "Simulating sudden road incident on [" << road_to_block << "]...\n\n";

    auto reroute_result = blockage_sim.simulateBlockageAndReroute(road_to_block, "L1", "L2", false);
    reroute_result.print(std::cout);

    // ========================================================================
    // SCENARIO 2: Road Clearance & Route Restoration
    // ========================================================================
    std::cout << "\n>>> SCENARIO 2: Road Clearance & Route Restoration\n";
    std::cout << "Emergency road maintenance completed on [" << road_to_block << "]. Reopening road...\n";
    bool reopened = blockage_sim.reopenRoad(road_to_block);
    std::cout << "Road [" << road_to_block << "] Reopen Status: "
              << (reopened ? "CONFIRMED OPEN" : "FAILED") << "\n";

    auto restored_route = smartresq::GraphAlgorithms::dijkstra(graph, "L1", "L2");
    std::cout << "Recomputing route using C++ Dijkstra after reopening:\n";
    restored_route.print(std::cout);

    // ========================================================================
    // SCENARIO 3: Severed Access (No Alternate Route Handling)
    // ========================================================================
    std::cout << "\n>>> SCENARIO 3: Severed Access (No Alternate Route Handling)\n";
    graph.addLocation({"ISOLATED_DISTRICT", "Isolated Valley", 18.5700, 73.8100});
    graph.addRoad("R_BRIDGE_ONLY", "L6", "ISOLATED_DISTRICT", 4.5, true);
    std::cout << "Added test location [ISOLATED_DISTRICT] accessible only via bridge [R_BRIDGE_ONLY]...\n";

    auto bridge_sim = blockage_sim.simulateBlockageAndReroute("R_BRIDGE_ONLY", "L1", "ISOLATED_DISTRICT", false);
    bridge_sim.print(std::cout);

    // ========================================================================
    // SCENARIO 4: Graph Reset Operation
    // ========================================================================
    std::cout << "\n>>> SCENARIO 4: Full Graph Reset Operation\n";
    std::cout << "Executing reset operation to clear all blockages and restore pristine network state...\n";
    blockage_sim.resetAllBlockages();
    std::cout << "Currently Blocked Roads count: " << blockage_sim.getBlockedRoads().size() << "\n";
    std::cout << "Original graph state successfully restored.\n\n";

    std::cout << "====================================================================\n";
    std::cout << "   Road Blockage Simulation Completed Successfully.                 \n";
    std::cout << "====================================================================\n";
    return 0;
}
