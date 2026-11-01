#pragma once

#include "Graph.hpp"
#include "GraphAlgorithms.hpp"
#include "EmergencyDispatcher.hpp"
#include "RoadBlockageSimulator.hpp"
#include "JsonUtils.hpp"
#include <string>

namespace smartresq {

/**
 * @brief Top-level JSON IPC Bridge for C++ Engine <-> Python Flask communication.
 */
class EngineApi {
public:
    explicit EngineApi(const std::string& data_dir);

    /**
     * @brief Processes a JSON request string and returns a JSON response string.
     */
    std::string processRequest(const std::string& json_input);

private:
    std::string data_dir_;
    CityGraph graph_;
    EmergencyDispatcher dispatcher_;

    void setupGraphWithState(const JsonValue& root);
    std::string handleDisplayGraph(const JsonValue& root);
    std::string handleRoute(const JsonValue& root);
    std::string handleAmbulanceSelect(const JsonValue& root);
    std::string handleHospitalSelect(const JsonValue& root);
    std::string handleRoadBlock(const JsonValue& root);
    std::string handleRoadReopen(const JsonValue& root);
    std::string handleReset(const JsonValue& root);

    static std::string makeError(const std::string& message, int code = 400);
};

} // namespace smartresq

