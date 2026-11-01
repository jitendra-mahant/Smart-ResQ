#include "Graph.hpp"
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>

namespace smartresq {

const std::vector<RoadEdge> CityGraph::empty_edges_{};

// ============================================================================
// Vertex Management
// ============================================================================

bool CityGraph::addLocation(const LocationNode& node) {
    if (node.id.empty()) {
        return false;
    }
    // Reject duplicate location IDs
    if (locations_.find(node.id) != locations_.end()) {
        return false;
    }

    locations_[node.id] = node;
    adjacency_list_[node.id] = {};
    return true;
}

bool CityGraph::hasLocation(const std::string& id) const {
    return locations_.find(id) != locations_.end();
}

std::optional<LocationNode> CityGraph::getLocation(const std::string& id) const {
    auto it = locations_.find(id);
    if (it != locations_.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::unordered_map<std::string, LocationNode>& CityGraph::getAllLocations() const {
    return locations_;
}

// ============================================================================
// Edge Management
// ============================================================================

bool CityGraph::roadExistsBetween(const std::string& u, const std::string& v, const std::string& road_id) const {
    auto it = adjacency_list_.find(u);
    if (it == adjacency_list_.end()) return false;

    for (const auto& edge : it->second) {
        if (edge.destination_id == v || edge.road_id == road_id) {
            return true;
        }
    }
    return false;
}

bool CityGraph::addRoad(const std::string& road_id,
                        const std::string& source_id,
                        const std::string& destination_id,
                        double distance_km,
                        bool bidirectional) {
    // 1. Validation checks
    if (road_id.empty() || source_id.empty() || destination_id.empty()) {
        return false;
    }
    if (source_id == destination_id) {
        // Self-loops disallowed in city road network
        return false;
    }
    if (!hasLocation(source_id) || !hasLocation(destination_id)) {
        // Invalid vertex IDs
        return false;
    }
    if (distance_km <= 0.0) {
        // Distance must be strictly positive
        return false;
    }

    // 2. Prevent duplicate roads
    if (roadExistsBetween(source_id, destination_id, road_id)) {
        return false;
    }
    if (bidirectional && roadExistsBetween(destination_id, source_id, road_id)) {
        return false;
    }

    // 3. Insert forward edge
    adjacency_list_[source_id].push_back(RoadEdge{road_id, destination_id, distance_km, false});

    // 4. Insert reverse edge for bidirectional road
    if (bidirectional) {
        adjacency_list_[destination_id].push_back(RoadEdge{road_id, source_id, distance_km, false});
    }

    return true;
}

std::vector<std::string> CityGraph::getAdjacentLocations(const std::string& location_id,
                                                         bool include_blocked) const {
    std::vector<std::string> neighbors;
    auto it = adjacency_list_.find(location_id);
    if (it == adjacency_list_.end()) {
        return neighbors;
    }

    neighbors.reserve(it->second.size());
    for (const auto& edge : it->second) {
        if (!include_blocked && edge.is_blocked) {
            continue;
        }
        neighbors.push_back(edge.destination_id);
    }
    return neighbors;
}

std::vector<RoadEdge> CityGraph::getOutgoingRoads(const std::string& location_id,
                                                  bool include_blocked) const {
    auto it = adjacency_list_.find(location_id);
    if (it == adjacency_list_.end()) {
        return empty_edges_;
    }

    if (include_blocked) {
        return it->second;
    }

    std::vector<RoadEdge> active_edges;
    active_edges.reserve(it->second.size());
    for (const auto& edge : it->second) {
        if (!edge.is_blocked) {
            active_edges.push_back(edge);
        }
    }
    return active_edges;
}

// ============================================================================
// Road Blocking & Reopening
// ============================================================================

bool CityGraph::blockRoad(const std::string& road_id) {
    if (road_id.empty()) return false;
    bool modified = false;

    for (auto& [_, edges] : adjacency_list_) {
        for (auto& edge : edges) {
            if (edge.road_id == road_id && !edge.is_blocked) {
                edge.is_blocked = true;
                modified = true;
            }
        }
    }
    return modified;
}

bool CityGraph::blockRoad(const std::string& source_id, const std::string& destination_id) {
    if (!hasLocation(source_id) || !hasLocation(destination_id)) {
        return false;
    }

    bool modified = false;
    std::string matched_road_id;

    for (auto& edge : adjacency_list_[source_id]) {
        if (edge.destination_id == destination_id) {
            edge.is_blocked = true;
            matched_road_id = edge.road_id;
            modified = true;
            break;
        }
    }

    // If matching edge found, block reverse direction as well
    if (modified && !matched_road_id.empty()) {
        for (auto& edge : adjacency_list_[destination_id]) {
            if (edge.destination_id == source_id && edge.road_id == matched_road_id) {
                edge.is_blocked = true;
                break;
            }
        }
    }

    return modified;
}

bool CityGraph::reopenRoad(const std::string& road_id) {
    if (road_id.empty()) return false;
    bool modified = false;

    for (auto& [_, edges] : adjacency_list_) {
        for (auto& edge : edges) {
            if (edge.road_id == road_id && edge.is_blocked) {
                edge.is_blocked = false;
                modified = true;
            }
        }
    }
    return modified;
}

bool CityGraph::reopenRoad(const std::string& source_id, const std::string& destination_id) {
    if (!hasLocation(source_id) || !hasLocation(destination_id)) {
        return false;
    }

    bool modified = false;
    std::string matched_road_id;

    for (auto& edge : adjacency_list_[source_id]) {
        if (edge.destination_id == destination_id) {
            edge.is_blocked = false;
            matched_road_id = edge.road_id;
            modified = true;
            break;
        }
    }

    if (modified && !matched_road_id.empty()) {
        for (auto& edge : adjacency_list_[destination_id]) {
            if (edge.destination_id == source_id && edge.road_id == matched_road_id) {
                edge.is_blocked = false;
                break;
            }
        }
    }

    return modified;
}

bool CityGraph::isRoadBlocked(const std::string& road_id) const {
    for (const auto& [_, edges] : adjacency_list_) {
        for (const auto& edge : edges) {
            if (edge.road_id == road_id) {
                return edge.is_blocked;
            }
        }
    }
    return false;
}

bool CityGraph::isRoadBlocked(const std::string& source_id, const std::string& destination_id) const {
    auto it = adjacency_list_.find(source_id);
    if (it == adjacency_list_.end()) return false;

    for (const auto& edge : it->second) {
        if (edge.destination_id == destination_id) {
            return edge.is_blocked;
        }
    }
    return false;
}

bool CityGraph::hasRoad(const std::string& road_id) const {
    for (const auto& [_, edges] : adjacency_list_) {
        for (const auto& edge : edges) {
            if (edge.road_id == road_id) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> CityGraph::getBlockedRoads() const {
    std::unordered_set<std::string> blocked_set;
    for (const auto& [_, edges] : adjacency_list_) {
        for (const auto& edge : edges) {
            if (edge.is_blocked) {
                blocked_set.insert(edge.road_id);
            }
        }
    }
    return std::vector<std::string>(blocked_set.begin(), blocked_set.end());
}

void CityGraph::reopenAllRoads() {
    for (auto& [_, edges] : adjacency_list_) {
        for (auto& edge : edges) {
            edge.is_blocked = false;
        }
    }
}

// ============================================================================
// Connectivity Checks
// ============================================================================

bool CityGraph::hasPath(const std::string& start_id,
                        const std::string& target_id,
                        bool respect_blockage) const {
    if (!hasLocation(start_id) || !hasLocation(target_id)) {
        return false;
    }
    if (start_id == target_id) {
        return true;
    }

    std::unordered_set<std::string> visited;
    std::queue<std::string> q;

    visited.insert(start_id);
    q.push(start_id);

    while (!q.empty()) {
        std::string curr = q.front();
        q.pop();

        if (curr == target_id) {
            return true;
        }

        auto it = adjacency_list_.find(curr);
        if (it != adjacency_list_.end()) {
            for (const auto& edge : it->second) {
                if (respect_blockage && edge.is_blocked) {
                    continue;
                }
                if (visited.find(edge.destination_id) == visited.end()) {
                    visited.insert(edge.destination_id);
                    q.push(edge.destination_id);
                }
            }
        }
    }

    return false;
}

bool CityGraph::isConnected(bool respect_blockage) const {
    if (locations_.empty()) {
        return true;
    }

    // Pick arbitrary first vertex as root
    const std::string& root = locations_.begin()->first;

    std::unordered_set<std::string> visited;
    std::queue<std::string> q;

    visited.insert(root);
    q.push(root);

    while (!q.empty()) {
        std::string curr = q.front();
        q.pop();

        auto it = adjacency_list_.find(curr);
        if (it != adjacency_list_.end()) {
            for (const auto& edge : it->second) {
                if (respect_blockage && edge.is_blocked) {
                    continue;
                }
                if (visited.find(edge.destination_id) == visited.end()) {
                    visited.insert(edge.destination_id);
                    q.push(edge.destination_id);
                }
            }
        }
    }

    // Graph is connected if every vertex in locations_ was visited
    return visited.size() == locations_.size();
}

// ============================================================================
// Inspection & Display
// ============================================================================

size_t CityGraph::roadCount(bool include_blocked) const {
    size_t count = 0;
    for (const auto& [_, edges] : adjacency_list_) {
        if (include_blocked) {
            count += edges.size();
        } else {
            for (const auto& edge : edges) {
                if (!edge.is_blocked) ++count;
            }
        }
    }
    return count;
}

void CityGraph::clear() {
    locations_.clear();
    adjacency_list_.clear();
}

void CityGraph::displayGraph(std::ostream& os) const {
    os << "====================================================================\n";
    os << "  SmartResQ City Graph (Adjacency List Representation)\n";
    os << "  Total Locations (Vertices): " << nodeCount() << "\n";
    os << "  Total Directed Edges:       " << roadCount(true)
       << " (" << roadCount(false) << " Active, "
       << (roadCount(true) - roadCount(false)) << " Blocked)\n";
    os << "  Global Connectivity Status: "
       << (isConnected(true) ? "[CONNECTED]" : "[DISCONNECTED (Check Blocked Roads)]") << "\n";
    os << "====================================================================\n";

    for (const auto& [id, node] : locations_) {
        os << "[" << id << "] " << node.name
           << " (Lat: " << std::fixed << std::setprecision(4) << node.latitude
           << ", Lon: " << node.longitude << ")\n";

        const auto& edges = adjacency_list_.at(id);
        if (edges.empty()) {
            os << "    (isolated vertex - no connections)\n";
        } else {
            for (const auto& edge : edges) {
                std::string dest_name = edge.destination_id;
                auto dest_node = getLocation(edge.destination_id);
                if (dest_node) dest_name = dest_node->name;

                os << "    --> [" << edge.destination_id << "] " << std::left << std::setw(28) << dest_name
                   << " | Road: " << std::setw(5) << edge.road_id
                   << " | Dist: " << std::fixed << std::setprecision(2) << std::setw(5) << edge.distance_km << " km"
                   << " | Status: " << (edge.is_blocked ? "[BLOCKED]" : "[ACTIVE]")
                   << "\n";
            }
        }
    }
    os << "====================================================================\n";
}

// ============================================================================
// Lightweight Zero-Dependency JSON Parser for City Graph
// ============================================================================

namespace {

// Minimal JSON Tokenizer
struct JsonToken {
    enum Type {
        ObjectOpen, ObjectClose, ArrayOpen, ArrayClose,
        Colon, Comma, StringVal, NumberVal, BoolVal, NullVal, EndOfStream
    } type;
    std::string str_val;
    double num_val{0.0};
};

class MiniJsonScanner {
public:
    explicit MiniJsonScanner(std::string text) : src_(std::move(text)), idx_(0) {}

    JsonToken nextToken() {
        skipWhitespace();
        if (idx_ >= src_.size()) {
            return {JsonToken::EndOfStream, "", 0.0};
        }

        char c = src_[idx_++];
        switch (c) {
            case '{': return {JsonToken::ObjectOpen, "{", 0.0};
            case '}': return {JsonToken::ObjectClose, "}", 0.0};
            case '[': return {JsonToken::ArrayOpen, "[", 0.0};
            case ']': return {JsonToken::ArrayClose, "]", 0.0};
            case ':': return {JsonToken::Colon, ":", 0.0};
            case ',': return {JsonToken::Comma, ",", 0.0};
            case '"': return scanString();
            default:
                if (c == '-' || std::isdigit(c)) {
                    --idx_;
                    return scanNumber();
                }
                if (std::isalpha(c)) {
                    --idx_;
                    return scanKeyword();
                }
                return {JsonToken::EndOfStream, "", 0.0};
        }
    }

private:
    void skipWhitespace() {
        while (idx_ < src_.size() && (std::isspace(src_[idx_]) || src_[idx_] == '\r' || src_[idx_] == '\n')) {
            ++idx_;
        }
    }

    JsonToken scanString() {
        std::string result;
        while (idx_ < src_.size()) {
            char ch = src_[idx_++];
            if (ch == '"') {
                break;
            }
            if (ch == '\\' && idx_ < src_.size()) {
                char escaped = src_[idx_++];
                if (escaped == '"') result.push_back('"');
                else if (escaped == '\\') result.push_back('\\');
                else if (escaped == 'n') result.push_back('\n');
                else if (escaped == 't') result.push_back('\t');
                else result.push_back(escaped);
            } else {
                result.push_back(ch);
            }
        }
        return {JsonToken::StringVal, result, 0.0};
    }

    JsonToken scanNumber() {
        size_t start = idx_;
        if (src_[idx_] == '-') ++idx_;
        while (idx_ < src_.size() && std::isdigit(src_[idx_])) ++idx_;
        if (idx_ < src_.size() && src_[idx_] == '.') {
            ++idx_;
            while (idx_ < src_.size() && std::isdigit(src_[idx_])) ++idx_;
        }
        if (idx_ < src_.size() && (src_[idx_] == 'e' || src_[idx_] == 'E')) {
            ++idx_;
            if (idx_ < src_.size() && (src_[idx_] == '+' || src_[idx_] == '-')) ++idx_;
            while (idx_ < src_.size() && std::isdigit(src_[idx_])) ++idx_;
        }
        std::string sub = src_.substr(start, idx_ - start);
        double val = std::stod(sub);
        return {JsonToken::NumberVal, sub, val};
    }

    JsonToken scanKeyword() {
        size_t start = idx_;
        while (idx_ < src_.size() && std::isalpha(src_[idx_])) ++idx_;
        std::string sub = src_.substr(start, idx_ - start);
        if (sub == "true" || sub == "false") {
            return {JsonToken::BoolVal, sub, 0.0};
        }
        return {JsonToken::NullVal, sub, 0.0};
    }

    std::string src_;
    size_t idx_{0};
};

// Generic JSON Node for representation
struct MiniJsonNode {
    enum Type { Null, Bool, Number, String, Array, Object } type{Null};
    std::string str;
    double num{0.0};
    std::vector<MiniJsonNode> arr;
    std::unordered_map<std::string, MiniJsonNode> obj;

    const MiniJsonNode* get(const std::string& key) const {
        auto it = obj.find(key);
        return (it != obj.end()) ? &it->second : nullptr;
    }
};

MiniJsonNode parseNode(MiniJsonScanner& scanner, JsonToken tok) {
    MiniJsonNode node;
    if (tok.type == JsonToken::StringVal) {
        node.type = MiniJsonNode::String;
        node.str = tok.str_val;
        return node;
    }
    if (tok.type == JsonToken::NumberVal) {
        node.type = MiniJsonNode::Number;
        node.num = tok.num_val;
        return node;
    }
    if (tok.type == JsonToken::BoolVal) {
        node.type = MiniJsonNode::Bool;
        node.str = tok.str_val;
        return node;
    }
    if (tok.type == JsonToken::NullVal) {
        node.type = MiniJsonNode::Null;
        return node;
    }
    if (tok.type == JsonToken::ObjectOpen) {
        node.type = MiniJsonNode::Object;
        while (true) {
            JsonToken key_tok = scanner.nextToken();
            if (key_tok.type == JsonToken::ObjectClose) break;
            if (key_tok.type == JsonToken::Comma) continue;
            if (key_tok.type != JsonToken::StringVal) break;

            JsonToken colon = scanner.nextToken();
            if (colon.type != JsonToken::Colon) break;

            JsonToken val_tok = scanner.nextToken();
            node.obj[key_tok.str_val] = parseNode(scanner, val_tok);
        }
        return node;
    }
    if (tok.type == JsonToken::ArrayOpen) {
        node.type = MiniJsonNode::Array;
        while (true) {
            JsonToken item_tok = scanner.nextToken();
            if (item_tok.type == JsonToken::ArrayClose) break;
            if (item_tok.type == JsonToken::Comma) continue;
            node.arr.push_back(parseNode(scanner, item_tok));
        }
        return node;
    }
    return node;
}

} // anonymous namespace

bool CityGraph::loadFromJsonFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "CityGraph::loadFromJsonFile: Unable to open file " << filepath << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    if (content.empty()) {
        return false;
    }

    MiniJsonScanner scanner(std::move(content));
    JsonToken root_tok = scanner.nextToken();
    if (root_tok.type != JsonToken::ObjectOpen) {
        return false;
    }

    MiniJsonNode root = parseNode(scanner, root_tok);

    // 1. Clear existing graph
    clear();

    // 2. Parse "locations"
    const MiniJsonNode* locations_arr = root.get("locations");
    if (locations_arr && locations_arr->type == MiniJsonNode::Array) {
        for (const auto& loc_item : locations_arr->arr) {
            if (loc_item.type != MiniJsonNode::Object) continue;
            const auto* id_node = loc_item.get("id");
            const auto* name_node = loc_item.get("name");
            const auto* lat_node = loc_item.get("latitude");
            const auto* lon_node = loc_item.get("longitude");

            if (id_node && name_node && lat_node && lon_node) {
                LocationNode loc;
                loc.id = id_node->str;
                loc.name = name_node->str;
                loc.latitude = lat_node->num;
                loc.longitude = lon_node->num;
                addLocation(loc);
            }
        }
    }

    // 3. Parse "roads"
    const MiniJsonNode* roads_arr = root.get("roads");
    if (roads_arr && roads_arr->type == MiniJsonNode::Array) {
        for (const auto& road_item : roads_arr->arr) {
            if (road_item.type != MiniJsonNode::Object) continue;
            const auto* id_node = road_item.get("id");
            const auto* src_node = road_item.get("source");
            const auto* dst_node = road_item.get("destination");
            const auto* dist_node = road_item.get("distance_km");

            if (id_node && src_node && dst_node && dist_node) {
                addRoad(id_node->str, src_node->str, dst_node->str, dist_node->num, true);
            }
        }
    }

    return (nodeCount() > 0 && roadCount() > 0);
}

} // namespace smartresq
