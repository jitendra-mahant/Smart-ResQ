#include "EngineApi.hpp"
#include <filesystem>
#include <algorithm>

namespace smartresq {

EngineApi::EngineApi(const std::string& data_dir)
    : data_dir_(data_dir), dispatcher_(graph_) {
    std::string graph_path = data_dir_ + "/city_graph.json";
    if (std::filesystem::exists(graph_path)) {
        graph_.loadFromJsonFile(graph_path);
    }

    std::string amb_path = data_dir_ + "/ambulances.json";
    if (std::filesystem::exists(amb_path)) {
        dispatcher_.loadAmbulancesFromJson(amb_path);
    }

    std::string hosp_path = data_dir_ + "/hospitals.json";
    if (std::filesystem::exists(hosp_path)) {
        dispatcher_.loadHospitalsFromJson(hosp_path);
    }
}

void EngineApi::setupGraphWithState(const JsonValue& root) {
    // 1. Reset all blockages and fleet
    graph_.reopenAllRoads();
    dispatcher_.resetFleetAvailability();

    // 2. Re-apply any active road blockages passed from state
    std::vector<std::string> blocked_roads = root.getStringArray("blocked_roads");
    for (const auto& r_id : blocked_roads) {
        graph_.blockRoad(r_id);
    }

    // 3. Re-apply any allocated ambulances passed from state
    std::vector<std::string> allocated_ambs = root.getStringArray("allocated_ambulances");
    for (const auto& a_id : allocated_ambs) {
        dispatcher_.setAmbulanceAvailability(a_id, false);
    }
}

std::string EngineApi::makeError(const std::string& message, int code) {
    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, "error", 0.0, false, {}, {}};
    out.obj_val["error"] = JsonValue{JsonValue::String, message, 0.0, false, {}, {}};
    out.obj_val["code"] = JsonValue{JsonValue::Number, "", static_cast<double>(code), false, {}, {}};
    return out.serialize();
}

std::string EngineApi::processRequest(const std::string& json_input) {
    if (json_input.empty()) {
        return makeError("Empty JSON request body.");
    }

    JsonValue root = JsonParser::parse(json_input);
    if (!root.isObject()) {
        return makeError("Invalid JSON input; expected root object.");
    }

    std::string action = root.getString("action");
    if (action.empty()) {
        action = root.getString("command");
    }

    if (action == "display_graph" || action == "get_city") {
        return handleDisplayGraph(root);
    } else if (action == "route") {
        return handleRoute(root);
    } else if (action == "ambulance_select" || action == "select_ambulance") {
        return handleAmbulanceSelect(root);
    } else if (action == "hospital_select" || action == "select_hospital") {
        return handleHospitalSelect(root);
    } else if (action == "road_block" || action == "block_road") {
        return handleRoadBlock(root);
    } else if (action == "road_reopen" || action == "reopen_road") {
        return handleRoadReopen(root);
    } else if (action == "reset") {
        return handleReset(root);
    } else {
        return makeError("Unknown action '" + action + "'. Supported actions: route, ambulance_select, hospital_select, road_block, road_reopen, display_graph, reset.");
    }
}

std::string EngineApi::handleDisplayGraph(const JsonValue& root) {
    setupGraphWithState(root);

    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, "success", 0.0, false, {}, {}};
    out.obj_val["node_count"] = JsonValue{JsonValue::Number, "", static_cast<double>(graph_.nodeCount()), false, {}, {}};
    out.obj_val["road_count"] = JsonValue{JsonValue::Number, "", static_cast<double>(graph_.roadCount(true)), false, {}, {}};
    out.obj_val["is_connected"] = JsonValue{JsonValue::Bool, "", 0.0, graph_.isConnected(true), {}, {}};

    // Blocked road IDs array
    JsonValue blocked_arr;
    blocked_arr.type = JsonValue::Array;
    for (const auto& r_id : graph_.getBlockedRoads()) {
        blocked_arr.arr_val.push_back(JsonValue{JsonValue::String, r_id, 0.0, false, {}, {}});
    }
    out.obj_val["blocked_roads"] = std::move(blocked_arr);

    return out.serialize();
}

std::string EngineApi::handleRoute(const JsonValue& root) {
    setupGraphWithState(root);

    std::string origin = root.getString("origin");
    std::string dest = root.getString("destination");
    std::string algo = root.getString("algorithm", "dijkstra");

    if (origin.empty() || dest.empty()) {
        return makeError("Missing required parameters 'origin' and 'destination'.");
    }

    if (!graph_.hasLocation(origin)) {
        return makeError("Origin location ID '" + origin + "' not found in graph.");
    }
    if (!graph_.hasLocation(dest)) {
        return makeError("Destination location ID '" + dest + "' not found in graph.");
    }

    bool use_bfs = (algo == "bfs" || algo == "BFS");
    RouteResult route = use_bfs ? GraphAlgorithms::bfs(graph_, origin, dest)
                                : GraphAlgorithms::dijkstra(graph_, origin, dest);

    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, "success", 0.0, false, {}, {}};
    out.obj_val["found"] = JsonValue{JsonValue::Bool, "", 0.0, route.found, {}, {}};
    out.obj_val["algorithm"] = JsonValue{JsonValue::String, route.algorithm_used, 0.0, false, {}, {}};
    out.obj_val["origin"] = JsonValue{JsonValue::String, route.source_id, 0.0, false, {}, {}};
    out.obj_val["destination"] = JsonValue{JsonValue::String, route.destination_id, 0.0, false, {}, {}};
    out.obj_val["distance_km"] = JsonValue{JsonValue::Number, "", route.found ? route.total_distance_km : -1.0, false, {}, {}};
    out.obj_val["hop_count"] = JsonValue{JsonValue::Number, "", static_cast<double>(route.hop_count), false, {}, {}};

    // path nodes
    JsonValue nodes_arr;
    nodes_arr.type = JsonValue::Array;
    for (const auto& n : route.path_nodes) {
        nodes_arr.arr_val.push_back(JsonValue{JsonValue::String, n, 0.0, false, {}, {}});
    }
    out.obj_val["path_nodes"] = std::move(nodes_arr);

    // road IDs
    JsonValue roads_arr;
    roads_arr.type = JsonValue::Array;
    for (const auto& r : route.road_ids) {
        roads_arr.arr_val.push_back(JsonValue{JsonValue::String, r, 0.0, false, {}, {}});
    }
    out.obj_val["road_ids"] = std::move(roads_arr);

    return out.serialize();
}

std::string EngineApi::handleAmbulanceSelect(const JsonValue& root) {
    setupGraphWithState(root);

    std::string incident = root.getString("incident_location_id");
    if (incident.empty()) {
        incident = root.getString("incident_location");
    }
    if (incident.empty()) {
        return makeError("Missing required parameter 'incident_location_id'.");
    }
    if (!graph_.hasLocation(incident)) {
        return makeError("Incident location ID '" + incident + "' not found in graph.");
    }

    // Run simple ambulance selection
    auto amb_res = dispatcher_.selectNearestAmbulance(incident, true);

    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, amb_res.success ? "success" : "failure", 0.0, false, {}, {}};
    out.obj_val["success"] = JsonValue{JsonValue::Bool, "", 0.0, amb_res.success, {}, {}};
    out.obj_val["message"] = JsonValue{JsonValue::String, amb_res.message, 0.0, false, {}, {}};

    if (amb_res.success) {
        out.obj_val["distance_km"] = JsonValue{JsonValue::Number, "", amb_res.distance_km, false, {}, {}};

        JsonValue amb_obj;
        amb_obj.type = JsonValue::Object;
        amb_obj.obj_val["id"] = JsonValue{JsonValue::String, amb_res.selected_ambulance.id, 0.0, false, {}, {}};
        amb_obj.obj_val["name"] = JsonValue{JsonValue::String, amb_res.selected_ambulance.name, 0.0, false, {}, {}};
        amb_obj.obj_val["location_id"] = JsonValue{JsonValue::String, amb_res.selected_ambulance.location_id, 0.0, false, {}, {}};
        out.obj_val["selected_ambulance"] = std::move(amb_obj);

        // route nodes
        JsonValue nodes_arr;
        nodes_arr.type = JsonValue::Array;
        for (const auto& n : amb_res.route.path_nodes) {
            nodes_arr.arr_val.push_back(JsonValue{JsonValue::String, n, 0.0, false, {}, {}});
        }
        out.obj_val["path_nodes"] = std::move(nodes_arr);

        // road ids
        JsonValue roads_arr;
        roads_arr.type = JsonValue::Array;
        for (const auto& r : amb_res.route.road_ids) {
            roads_arr.arr_val.push_back(JsonValue{JsonValue::String, r, 0.0, false, {}, {}});
        }
        out.obj_val["road_ids"] = std::move(roads_arr);
    }

    return out.serialize();
}

std::string EngineApi::handleHospitalSelect(const JsonValue& root) {
    setupGraphWithState(root);

    std::string patient_loc = root.getString("patient_location_id");
    if (patient_loc.empty()) {
        patient_loc = root.getString("incident_location_id");
    }
    if (patient_loc.empty()) {
        return makeError("Missing required parameter 'patient_location_id'.");
    }
    if (!graph_.hasLocation(patient_loc)) {
        return makeError("Patient location ID '" + patient_loc + "' not found in graph.");
    }

    auto hosp_res = dispatcher_.selectNearestHospital(patient_loc);

    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, hosp_res.success ? "success" : "failure", 0.0, false, {}, {}};
    out.obj_val["success"] = JsonValue{JsonValue::Bool, "", 0.0, hosp_res.success, {}, {}};
    out.obj_val["message"] = JsonValue{JsonValue::String, hosp_res.message, 0.0, false, {}, {}};

    if (hosp_res.success) {
        out.obj_val["distance_km"] = JsonValue{JsonValue::Number, "", hosp_res.distance_km, false, {}, {}};

        JsonValue hosp_obj;
        hosp_obj.type = JsonValue::Object;
        hosp_obj.obj_val["id"] = JsonValue{JsonValue::String, hosp_res.selected_hospital.id, 0.0, false, {}, {}};
        hosp_obj.obj_val["name"] = JsonValue{JsonValue::String, hosp_res.selected_hospital.name, 0.0, false, {}, {}};
        hosp_obj.obj_val["location_id"] = JsonValue{JsonValue::String, hosp_res.selected_hospital.location_id, 0.0, false, {}, {}};
        out.obj_val["selected_hospital"] = std::move(hosp_obj);

        // Path nodes
        JsonValue nodes_arr;
        nodes_arr.type = JsonValue::Array;
        for (const auto& n : hosp_res.route.path_nodes) {
            nodes_arr.arr_val.push_back(JsonValue{JsonValue::String, n, 0.0, false, {}, {}});
        }
        out.obj_val["path_nodes"] = std::move(nodes_arr);

        // Road IDs
        JsonValue roads_arr;
        roads_arr.type = JsonValue::Array;
        for (const auto& r : hosp_res.route.road_ids) {
            roads_arr.arr_val.push_back(JsonValue{JsonValue::String, r, 0.0, false, {}, {}});
        }
        out.obj_val["road_ids"] = std::move(roads_arr);

        // Alternatives array
        JsonValue alts_arr;
        alts_arr.type = JsonValue::Array;
        for (const auto& alt : hosp_res.alternatives) {
            JsonValue a_obj;
            a_obj.type = JsonValue::Object;
            a_obj.obj_val["id"] = JsonValue{JsonValue::String, alt.hospital.id, 0.0, false, {}, {}};
            a_obj.obj_val["name"] = JsonValue{JsonValue::String, alt.hospital.name, 0.0, false, {}, {}};
            a_obj.obj_val["location_id"] = JsonValue{JsonValue::String, alt.hospital.location_id, 0.0, false, {}, {}};
            a_obj.obj_val["distance_km"] = JsonValue{JsonValue::Number, "", alt.distance_km, false, {}, {}};
            alts_arr.arr_val.push_back(std::move(a_obj));
        }
        out.obj_val["alternatives"] = std::move(alts_arr);
    }

    return out.serialize();
}

std::string EngineApi::handleRoadBlock(const JsonValue& root) {
    setupGraphWithState(root);

    std::string road_id = root.getString("road_id");
    if (road_id.empty()) {
        return makeError("Missing required parameter 'road_id'.");
    }
    if (!graph_.hasRoad(road_id)) {
        return makeError("Road ID '" + road_id + "' does not exist in city graph.");
    }

    // Block road in the graph
    graph_.blockRoad(road_id);

    // Compute updated connectivity and blocked roads list
    std::vector<std::string> blocked_list = graph_.getBlockedRoads();

    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, "success", 0.0, false, {}, {}};
    out.obj_val["blocked_road_id"] = JsonValue{JsonValue::String, road_id, 0.0, false, {}, {}};
    out.obj_val["is_connected"] = JsonValue{JsonValue::Bool, "", 0.0, graph_.isConnected(true), {}, {}};
    out.obj_val["message"] = JsonValue{JsonValue::String, "Road [" + road_id + "] blocked successfully.", 0.0, false, {}, {}};

    JsonValue arr;
    arr.type = JsonValue::Array;
    for (const auto& r : blocked_list) {
        arr.arr_val.push_back(JsonValue{JsonValue::String, r, 0.0, false, {}, {}});
    }
    out.obj_val["all_blocked_roads"] = std::move(arr);

    return out.serialize();
}

std::string EngineApi::handleRoadReopen(const JsonValue& root) {
    setupGraphWithState(root);

    std::string road_id = root.getString("road_id");
    if (road_id.empty()) {
        return makeError("Missing required parameter 'road_id'.");
    }
    if (!graph_.hasRoad(road_id)) {
        return makeError("Road ID '" + road_id + "' does not exist in city graph.");
    }

    graph_.reopenRoad(road_id);
    std::vector<std::string> blocked_list = graph_.getBlockedRoads();

    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, "success", 0.0, false, {}, {}};
    out.obj_val["reopened_road_id"] = JsonValue{JsonValue::String, road_id, 0.0, false, {}, {}};
    out.obj_val["is_connected"] = JsonValue{JsonValue::Bool, "", 0.0, graph_.isConnected(true), {}, {}};
    out.obj_val["message"] = JsonValue{JsonValue::String, "Road [" + road_id + "] reopened successfully.", 0.0, false, {}, {}};

    JsonValue arr;
    arr.type = JsonValue::Array;
    for (const auto& r : blocked_list) {
        arr.arr_val.push_back(JsonValue{JsonValue::String, r, 0.0, false, {}, {}});
    }
    out.obj_val["all_blocked_roads"] = std::move(arr);

    return out.serialize();
}

std::string EngineApi::handleReset(const JsonValue& /*root*/) {
    graph_.reopenAllRoads();
    dispatcher_.resetFleetAvailability();

    JsonValue out;
    out.type = JsonValue::Object;
    out.obj_val["status"] = JsonValue{JsonValue::String, "success", 0.0, false, {}, {}};
    out.obj_val["message"] = JsonValue{JsonValue::String, "Graph network and ambulance fleet reset to original state.", 0.0, false, {}, {}};

    JsonValue arr;
    arr.type = JsonValue::Array;
    out.obj_val["all_blocked_roads"] = std::move(arr);

    return out.serialize();
}

} // namespace smartresq
