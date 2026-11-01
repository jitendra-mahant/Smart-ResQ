#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <iostream>

namespace smartresq {

/**
 * @brief Represents a physical intersection, landmark, or facility vertex in the road network.
 */
struct LocationNode {
    std::string id;          // Unique identifier (e.g. "L1")
    std::string name;        // Human-readable location name
    double latitude{0.0};    // Geographic latitude
    double longitude{0.0};   // Geographic longitude
};

/**
 * @brief Represents a road connection between two locations.
 */
struct RoadEdge {
    std::string road_id;        // Unique road identifier (e.g. "R01")
    std::string destination_id; // Adjacent location vertex ID
    double distance_km{0.0};    // Road length in kilometers (> 0.0)
    bool is_blocked{false};     // True if road is blocked/impassable
};

/**
 * @brief Adjacency List Graph representing the city road network.
 * 
 * ============================================================================
 * TIME & SPACE COMPLEXITY SPECIFICATION:
 * ============================================================================
 * - Space Complexity:
 *     O(|V| + |E|) where |V| is number of location vertices and |E| is road edges.
 * 
 * - Time Complexity of Operations:
 *     1. addLocation(node):             O(1) average hash insert
 *     2. hasLocation(id):               O(1) average hash lookup
 *     3. getLocation(id):               O(1) average hash lookup
 *     4. addRoad(road_id, u, v, d):     O(deg(u) + deg(v)) duplicate check, O(1) amortized insertion
 *     5. getAdjacentLocations(u):       O(deg(u)) neighbor iteration
 *     6. getOutgoingRoads(u):           O(deg(u)) edge list filter
 *     7. blockRoad(road_id):            O(|E|) search and flag across adjacency lists
 *     8. blockRoad(u, v):               O(deg(u) + deg(v)) target edge search
 *     9. reopenRoad(road_id):           O(|E|) search and reset flag
 *    10. reopenRoad(u, v):              O(deg(u) + deg(v)) target edge search
 *    11. isRoadBlocked(road_id):        O(|E|) lookup
 *    12. isRoadBlocked(u, v):           O(deg(u)) lookup
 *    13. hasPath(u, v):                 O(|V| + |E|) Breadth-First Search (BFS)
 *    14. isConnected():                 O(|V| + |E|) BFS traversal from arbitrary root
 *    15. displayGraph():                O(|V| + |E|) full graph traversal
 *    16. loadFromJsonFile(path):        O(|V| + |E|) linear token stream parsing
 * ============================================================================
 */
class CityGraph {
public:
    CityGraph() = default;

    // --- Vertex Management ---
    /**
     * @brief Adds a new location vertex to the graph.
     * @param node The LocationNode struct containing ID, name, lat, and lon.
     * @return true if successfully added; false if ID is empty or already exists.
     */
    bool addLocation(const LocationNode& node);

    /**
     * @brief Checks if a location vertex exists in the graph.
     * @param id Location identifier.
     */
    bool hasLocation(const std::string& id) const;

    /**
     * @brief Retrieves location metadata if it exists.
     * @param id Location identifier.
     */
    std::optional<LocationNode> getLocation(const std::string& id) const;

    /**
     * @brief Returns a reference to all registered location vertices.
     */
    const std::unordered_map<std::string, LocationNode>& getAllLocations() const;

    // --- Edge Management ---
    /**
     * @brief Adds a road edge between two vertices.
     * @param road_id Unique road identifier.
     * @param source_id Source location ID.
     * @param destination_id Destination location ID.
     * @param distance_km Road length in kilometers (must be > 0.0).
     * @param bidirectional If true, adds reverse edge from destination to source.
     * @return true if added; false if invalid IDs, non-positive distance, or duplicate road.
     */
    bool addRoad(const std::string& road_id,
                 const std::string& source_id,
                 const std::string& destination_id,
                 double distance_km,
                 bool bidirectional = true);

    /**
     * @brief Retrieves adjacent location IDs.
     * @param location_id Origin vertex ID.
     * @param include_blocked If false, excludes destinations via blocked roads.
     * @return Vector of adjacent location vertex IDs.
     */
    std::vector<std::string> getAdjacentLocations(const std::string& location_id,
                                                  bool include_blocked = false) const;

    /**
     * @brief Retrieves outgoing road edges from a location.
     * @param location_id Origin vertex ID.
     * @param include_blocked If false, filters out blocked edges.
     * @return Vector of RoadEdge objects.
     */
    std::vector<RoadEdge> getOutgoingRoads(const std::string& location_id,
                                          bool include_blocked = false) const;

    // --- Road Blocking & Reopening ---
    /**
     * @brief Blocks a road by its road_id (both directions if bidirectional).
     * @param road_id Identifier of the road to block.
     * @return true if at least one matching road was found and blocked; false otherwise.
     */
    bool blockRoad(const std::string& road_id);

    /**
     * @brief Blocks road(s) connecting source and destination.
     * @param source_id Start location ID.
     * @param destination_id End location ID.
     * @return true if matching road was found and blocked; false otherwise.
     */
    bool blockRoad(const std::string& source_id, const std::string& destination_id);

    /**
     * @brief Reopens a previously blocked road by its road_id.
     * @param road_id Identifier of the road to reopen.
     * @return true if at least one matching road was found and reopened; false otherwise.
     */
    bool reopenRoad(const std::string& road_id);

    /**
     * @brief Reopens road(s) connecting source and destination.
     * @param source_id Start location ID.
     * @param destination_id End location ID.
     * @return true if matching road was found and reopened; false otherwise.
     */
    bool reopenRoad(const std::string& source_id, const std::string& destination_id);

    /**
     * @brief Checks if a road is currently blocked by road_id.
     */
    bool isRoadBlocked(const std::string& road_id) const;

    /**
     * @brief Checks if a direct road between source and destination is blocked.
     */
    bool isRoadBlocked(const std::string& source_id, const std::string& destination_id) const;

    /**
     * @brief Checks if a road with the given road_id exists in the graph.
     */
    bool hasRoad(const std::string& road_id) const;

    /**
     * @brief Retrieves a list of unique road IDs that are currently marked blocked.
     */
    std::vector<std::string> getBlockedRoads() const;

    /**
     * @brief Restores the entire graph to its original unblocked state by reopening all roads.
     */
    void reopenAllRoads();

    // --- Connectivity Checks ---
    /**
     * @brief Checks if there is a path connecting start_id and target_id (via BFS).
     * @param start_id Start vertex ID.
     * @param target_id Target vertex ID.
     * @param respect_blockage If true, blocked roads are considered impassable.
     * @return true if a valid path exists; false otherwise.
     */
    bool hasPath(const std::string& start_id,
                 const std::string& target_id,
                 bool respect_blockage = true) const;

    /**
     * @brief Checks if all vertices form a single connected component (via BFS).
     * @param respect_blockage If true, blocked roads are treated as severed.
     * @return true if every vertex is reachable from every other vertex.
     */
    bool isConnected(bool respect_blockage = true) const;

    // --- Inspection & Display ---
    /**
     * @brief Displays the adjacency list graph structure to the specified output stream.
     */
    void displayGraph(std::ostream& os = std::cout) const;

    size_t nodeCount() const noexcept { return locations_.size(); }
    size_t roadCount(bool include_blocked = true) const;

    void clear();

    // --- JSON Dataset Loading ---
    /**
     * @brief Loads locations and roads from a city graph JSON file without external libraries.
     * @param filepath Path to city_graph.json
     * @return true if successfully loaded and parsed, false on I/O or syntax error.
     */
    bool loadFromJsonFile(const std::string& filepath);

private:
    std::unordered_map<std::string, LocationNode> locations_;
    std::unordered_map<std::string, std::vector<RoadEdge>> adjacency_list_;
    static const std::vector<RoadEdge> empty_edges_;

    bool roadExistsBetween(const std::string& u, const std::string& v, const std::string& road_id) const;
};

} // namespace smartresq
