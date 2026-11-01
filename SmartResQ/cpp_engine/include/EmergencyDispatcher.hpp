#pragma once

#include "Graph.hpp"
#include "GraphAlgorithms.hpp"
#include <string>
#include <vector>
#include <limits>
#include <iostream>

namespace smartresq {

/**
 * @brief Represents an emergency vehicle in the simulation.
 */
struct Ambulance {
    std::string id;             // Unique identifier (e.g. "AMB-01")
    std::string location_id;    // Current vertex location ID (e.g. "L5")
    bool is_available{true};    // Availability status
    std::string name;           // Optional friendly name
};

/**
 * @brief Represents a destination hospital facility in the city.
 */
struct Hospital {
    std::string id;             // Unique identifier (e.g. "HOSP-01")
    std::string name;           // Hospital facility name
    std::string location_id;    // Vertex location ID (e.g. "L2")
};

/**
 * @brief Evaluated hospital candidate with route and distance.
 */
struct HospitalCandidate {
    Hospital hospital;
    double distance_km{std::numeric_limits<double>::infinity()};
    RouteResult route;
    bool reachable{false};
};

/**
 * @brief Result of selecting the nearest ambulance.
 */
struct AmbulanceSelectionResult {
    bool success{false};
    Ambulance selected_ambulance;
    double distance_km{std::numeric_limits<double>::infinity()};
    RouteResult route;
    std::string message;

    void print(std::ostream& os = std::cout) const;
};

/**
 * @brief Result of selecting the nearest hospital and alternative options.
 */
struct HospitalSelectionResult {
    bool success{false};
    Hospital selected_hospital;
    double distance_km{std::numeric_limits<double>::infinity()};
    RouteResult route;
    std::vector<HospitalCandidate> alternatives; // Other reachable hospitals sorted by distance
    std::string message;

    void print(std::ostream& os = std::cout) const;
};

/**
 * @brief Full emergency mission dispatch result.
 */
struct FullDispatchResult {
    bool success{false};
    std::string incident_location_id;
    AmbulanceSelectionResult ambulance_result;
    HospitalSelectionResult hospital_result;
    double total_mission_distance_km{0.0};
    std::string message;

    void print(std::ostream& os = std::cout) const;
};

/**
 * @brief Simple Emergency Dispatcher coordinating ambulance and hospital selection.
 * 
 * ============================================================================
 * COMPLEXITY SPECIFICATION:
 * ============================================================================
 * Let:
 *   |V| = Number of locations in the graph.
 *   |E| = Number of road segments in the graph.
 *   K   = Number of registered ambulances.
 *   H   = Number of registered hospitals.
 * 
 * 1. selectNearestAmbulance:
 *    - Time Complexity:  O(K * (|V| + |E|) log |V|)
 *      Evaluates distance from each available ambulance to the incident location
 *      using Dijkstra's shortest path algorithm.
 *    - Space Complexity: O(|V|)
 * 
 * 2. selectNearestHospital:
 *    - Time Complexity:  O(H * (|V| + |E|) log |V|)
 *      Evaluates distance from the patient incident to each hospital facility.
 *    - Space Complexity: O(|V| + H) to sort and store alternative candidates.
 * 
 * 3. Double-Allocation Prevention:
 *    - Time Complexity:  O(1)
 *      Flags selected ambulance's is_available = false immediately upon dispatch.
 * 
 * Total Memory Footprint: O(|V| + |E| + K + H)
 * ============================================================================
 */
class EmergencyDispatcher {
public:
    explicit EmergencyDispatcher(const CityGraph& graph);

    // --- Fleet & Facility Registration ---
    void addAmbulance(const Ambulance& ambulance);
    void addHospital(const Hospital& hospital);

    void setAmbulanceAvailability(const std::string& ambulance_id, bool available);
    bool isAmbulanceAvailable(const std::string& ambulance_id) const;

    const std::vector<Ambulance>& getAmbulances() const noexcept { return ambulances_; }
    const std::vector<Hospital>& getHospitals() const noexcept { return hospitals_; }

    void clearAmbulances() noexcept { ambulances_.clear(); }
    void clearHospitals() noexcept { hospitals_.clear(); }

    /**
     * @brief Resets all ambulances back to Available (is_available = true).
     */
    void resetFleetAvailability();

    // --- JSON Loading ---
    bool loadAmbulancesFromJson(const std::string& filepath);
    bool loadHospitalsFromJson(const std::string& filepath);

    // --- Selection Logic ---
    /**
     * @brief Selects the nearest available and reachable ambulance to the incident.
     * @param incident_location_id The location where emergency happened.
     * @param auto_allocate If true, marks the chosen ambulance as unavailable.
     * @return AmbulanceSelectionResult containing selected ambulance, distance, and route.
     */
    AmbulanceSelectionResult selectNearestAmbulance(const std::string& incident_location_id,
                                                   bool auto_allocate = true);

    /**
     * @brief Selects the nearest reachable hospital from the patient location.
     *        Also computes and returns alternative reachable hospitals sorted by distance.
     * @param patient_location_id The location of the patient.
     * @return HospitalSelectionResult containing chosen hospital and all alternatives.
     */
    HospitalSelectionResult selectNearestHospital(const std::string& patient_location_id);

    /**
     * @brief Coordinates complete emergency dispatch (pickup leg + hospital transfer leg).
     * @param incident_location_id Emergency incident location.
     * @return FullDispatchResult with end-to-end metrics.
     */
    FullDispatchResult dispatchEmergency(const std::string& incident_location_id);

private:
    const CityGraph& graph_;
    std::vector<Ambulance> ambulances_;
    std::vector<Hospital> hospitals_;
};

} // namespace smartresq

