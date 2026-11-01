#include "EmergencyDispatcher.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace smartresq {

// ============================================================================
// Output Formatters
// ============================================================================

void AmbulanceSelectionResult::print(std::ostream& os) const {
    os << "====================================================================\n";
    os << "  AMBULANCE DISPATCH SELECTION RESULT\n";
    os << "====================================================================\n";
    if (!success) {
        os << "  Status:  FAILED - " << message << "\n";
        os << "====================================================================\n";
        return;
    }

    os << "  Selected Ambulance:  " << selected_ambulance.id
       << (selected_ambulance.name.empty() ? "" : " (" + selected_ambulance.name + ")") << "\n";
    os << "  Starting Station:    [" << selected_ambulance.location_id << "]\n";
    os << "  Travel Distance:     " << std::fixed << std::setprecision(2) << distance_km << " km\n";
    os << "  Allocation Status:   " << (selected_ambulance.is_available ? "AVAILABLE" : "ALLOCATED [DISPATCHED]") << "\n";
    os << "  Response Route:      ";
    for (size_t i = 0; i < route.path_nodes.size(); ++i) {
        os << route.path_nodes[i] << (i + 1 < route.path_nodes.size() ? " -> " : "");
    }
    os << "\n  Road Segments:       ";
    for (size_t i = 0; i < route.road_ids.size(); ++i) {
        os << route.road_ids[i] << (i + 1 < route.road_ids.size() ? " -> " : "");
    }
    os << "\n====================================================================\n";
}

void HospitalSelectionResult::print(std::ostream& os) const {
    os << "====================================================================\n";
    os << "  HOSPITAL SELECTION & ALTERNATIVE FACILITIES\n";
    os << "====================================================================\n";
    if (!success) {
        os << "  Status:  FAILED - " << message << "\n";
        os << "====================================================================\n";
        return;
    }

    os << "  Primary Hospital:    " << selected_hospital.id << " (" << selected_hospital.name << ")\n";
    os << "  Hospital Location:   [" << selected_hospital.location_id << "]\n";
    os << "  Transfer Distance:   " << std::fixed << std::setprecision(2) << distance_km << " km\n";
    os << "  Transfer Route:      ";
    for (size_t i = 0; i < route.path_nodes.size(); ++i) {
        os << route.path_nodes[i] << (i + 1 < route.path_nodes.size() ? " -> " : "");
    }
    os << "\n";

    if (!alternatives.empty()) {
        os << "\n  Alternative Reachable Hospitals Evaluated:\n";
        for (size_t i = 0; i < alternatives.size(); ++i) {
            const auto& alt = alternatives[i];
            os << "    " << (i + 1) << ". " << std::left << std::setw(10) << alt.hospital.id
               << " | " << std::setw(28) << alt.hospital.name
               << " | Loc: [" << alt.hospital.location_id << "]"
               << " | Dist: " << std::fixed << std::setprecision(2) << alt.distance_km << " km\n";
        }
    } else {
        os << "  (No other reachable alternative hospitals found)\n";
    }
    os << "====================================================================\n";
}

void FullDispatchResult::print(std::ostream& os) const {
    os << "\n####################################################################\n";
    os << "  SMARTRESQ FULL EMERGENCY MISSION REPORT\n";
    os << "  Incident Location: [" << incident_location_id << "]\n";
    os << "####################################################################\n";
    if (!success) {
        os << "  MISSION STATUS: FAILED (" << message << ")\n";
        os << "####################################################################\n";
        return;
    }

    os << "  1. PICKUP LEG: Nearest Ambulance\n";
    os << "     - Ambulance: " << ambulance_result.selected_ambulance.id << " ("
       << ambulance_result.selected_ambulance.name << ")\n";
    os << "     - Distance:  " << std::fixed << std::setprecision(2) << ambulance_result.distance_km << " km\n";

    os << "  2. TRANSFER LEG: Nearest Hospital\n";
    os << "     - Hospital:  " << hospital_result.selected_hospital.id << " ("
       << hospital_result.selected_hospital.name << ")\n";
    os << "     - Distance:  " << std::fixed << std::setprecision(2) << hospital_result.distance_km << " km\n";

    os << "  TOTAL MISSION DISTANCE: " << std::fixed << std::setprecision(2) << total_mission_distance_km << " km\n";
    os << "####################################################################\n\n";
}

// ============================================================================
// Constructor & Registration
// ============================================================================

EmergencyDispatcher::EmergencyDispatcher(const CityGraph& graph)
    : graph_(graph) {}

void EmergencyDispatcher::addAmbulance(const Ambulance& ambulance) {
    ambulances_.push_back(ambulance);
}

void EmergencyDispatcher::addHospital(const Hospital& hospital) {
    hospitals_.push_back(hospital);
}

void EmergencyDispatcher::setAmbulanceAvailability(const std::string& ambulance_id, bool available) {
    for (auto& amb : ambulances_) {
        if (amb.id == ambulance_id) {
            amb.is_available = available;
            break;
        }
    }
}

bool EmergencyDispatcher::isAmbulanceAvailable(const std::string& ambulance_id) const {
    for (const auto& amb : ambulances_) {
        if (amb.id == ambulance_id) {
            return amb.is_available;
        }
    }
    return false;
}

void EmergencyDispatcher::resetFleetAvailability() {
    for (auto& amb : ambulances_) {
        amb.is_available = true;
    }
}

// ============================================================================
// Ambulance Selection
// ============================================================================

AmbulanceSelectionResult EmergencyDispatcher::selectNearestAmbulance(const std::string& incident_location_id,
                                                                     bool auto_allocate) {
    AmbulanceSelectionResult result;

    if (!graph_.hasLocation(incident_location_id)) {
        result.message = "Emergency location ID [" + incident_location_id + "] not found in city graph.";
        return result;
    }

    int best_index = -1;
    double min_distance = std::numeric_limits<double>::infinity();
    RouteResult best_route;

    for (size_t i = 0; i < ambulances_.size(); ++i) {
        const auto& amb = ambulances_[i];

        // 1. Ignore unavailable ambulances
        if (!amb.is_available) {
            continue;
        }

        // 2. Validate ambulance station exists in graph
        if (!graph_.hasLocation(amb.location_id)) {
            continue;
        }

        // 3. Compute distance using Dijkstra from ambulance to incident location
        RouteResult route = GraphAlgorithms::dijkstra(graph_, amb.location_id, incident_location_id);

        // 4. Ignore unreachable ambulances
        if (!route.found) {
            continue;
        }

        // 5. Select minimum distance
        if (route.total_distance_km < min_distance) {
            min_distance = route.total_distance_km;
            best_index = static_cast<int>(i);
            best_route = std::move(route);
        }
    }

    if (best_index == -1) {
        result.message = "No available and reachable ambulance found for incident location [" + incident_location_id + "].";
        return result;
    }

    // Allocate selected ambulance to prevent double allocation in the same simulation
    if (auto_allocate) {
        ambulances_[best_index].is_available = false;
    }

    result.success = true;
    result.selected_ambulance = ambulances_[best_index];
    result.distance_km = min_distance;
    result.route = std::move(best_route);
    result.message = "Nearest ambulance allocated successfully.";

    return result;
}

// ============================================================================
// Hospital Selection
// ============================================================================

HospitalSelectionResult EmergencyDispatcher::selectNearestHospital(const std::string& patient_location_id) {
    HospitalSelectionResult result;

    if (!graph_.hasLocation(patient_location_id)) {
        result.message = "Patient location ID [" + patient_location_id + "] not found in city graph.";
        return result;
    }

    std::vector<HospitalCandidate> evaluated_candidates;

    for (const auto& hosp : hospitals_) {
        if (!graph_.hasLocation(hosp.location_id)) {
            continue;
        }

        // Calculate distance from patient location to each hospital using Dijkstra
        RouteResult route = GraphAlgorithms::dijkstra(graph_, patient_location_id, hosp.location_id);

        // Ignore unreachable hospitals
        if (!route.found) {
            continue;
        }

        evaluated_candidates.push_back({hosp, route.total_distance_km, std::move(route), true});
    }

    if (evaluated_candidates.empty()) {
        result.message = "No reachable hospital facility found from location [" + patient_location_id + "].";
        return result;
    }

    // Sort evaluated hospitals ascending by distance
    std::sort(evaluated_candidates.begin(), evaluated_candidates.end(),
              [](const HospitalCandidate& a, const HospitalCandidate& b) {
                  return a.distance_km < b.distance_km;
              });

    // Primary nearest hospital is index 0
    result.success = true;
    result.selected_hospital = evaluated_candidates[0].hospital;
    result.distance_km = evaluated_candidates[0].distance_km;
    result.route = evaluated_candidates[0].route;

    // Remaining candidates become alternative choices
    for (size_t i = 1; i < evaluated_candidates.size(); ++i) {
        result.alternatives.push_back(evaluated_candidates[i]);
    }

    result.message = "Nearest hospital identified with alternative facilities evaluated.";
    return result;
}

// ============================================================================
// Full Mission Dispatch Coordination
// ============================================================================

FullDispatchResult EmergencyDispatcher::dispatchEmergency(const std::string& incident_location_id) {
    FullDispatchResult mission;
    mission.incident_location_id = incident_location_id;

    // 1. Select and allocate nearest ambulance
    mission.ambulance_result = selectNearestAmbulance(incident_location_id, true);
    if (!mission.ambulance_result.success) {
        mission.success = false;
        mission.message = mission.ambulance_result.message;
        return mission;
    }

    // 2. Select nearest hospital and compute alternatives
    mission.hospital_result = selectNearestHospital(incident_location_id);
    if (!mission.hospital_result.success) {
        mission.success = false;
        mission.message = mission.hospital_result.message;
        return mission;
    }

    mission.success = true;
    mission.total_mission_distance_km = mission.ambulance_result.distance_km +
                                       mission.hospital_result.distance_km;
    mission.message = "Emergency dispatch executed successfully.";
    return mission;
}

// ============================================================================
// Lightweight Zero-Dependency JSON Loading for Ambulances and Hospitals
// ============================================================================

namespace {

struct JsonTok {
    enum Type { ObjOpen, ObjClose, ArrOpen, ArrClose, Colon, Comma, StrVal, NumVal, BoolVal, Eof } type;
    std::string str;
    double num{0.0};
};

class DispatcherJsonScanner {
public:
    explicit DispatcherJsonScanner(std::string text) : s_(std::move(text)), p_(0) {}

    JsonTok next() {
        while (p_ < s_.size() && (std::isspace(s_[p_]) || s_[p_] == '\r' || s_[p_] == '\n')) ++p_;
        if (p_ >= s_.size()) return {JsonTok::Eof, "", 0.0};

        char c = s_[p_++];
        if (c == '{') return {JsonTok::ObjOpen, "{", 0.0};
        if (c == '}') return {JsonTok::ObjClose, "}", 0.0};
        if (c == '[') return {JsonTok::ArrOpen, "[", 0.0};
        if (c == ']') return {JsonTok::ArrClose, "]", 0.0};
        if (c == ':') return {JsonTok::Colon, ":", 0.0};
        if (c == ',') return {JsonTok::Comma, ",", 0.0};

        if (c == '"') {
            std::string str;
            while (p_ < s_.size()) {
                char ch = s_[p_++];
                if (ch == '"') break;
                if (ch == '\\' && p_ < s_.size()) str.push_back(s_[p_++]);
                else str.push_back(ch);
            }
            return {JsonTok::StrVal, str, 0.0};
        }

        if (std::isdigit(c) || c == '-') {
            size_t start = p_ - 1;
            while (p_ < s_.size() && (std::isdigit(s_[p_]) || s_[p_] == '.' || s_[p_] == 'e' || s_[p_] == 'E' || s_[p_] == '+' || s_[p_] == '-')) ++p_;
            std::string sub = s_.substr(start, p_ - start);
            return {JsonTok::NumVal, sub, std::stod(sub)};
        }

        if (std::isalpha(c)) {
            size_t start = p_ - 1;
            while (p_ < s_.size() && std::isalpha(s_[p_])) ++p_;
            std::string sub = s_.substr(start, p_ - start);
            return {JsonTok::BoolVal, sub, 0.0};
        }

        return {JsonTok::Eof, "", 0.0};
    }

private:
    std::string s_;
    size_t p_{0};
};

struct JsonItem {
    enum Type { Null, String, Number, Array, Object } type{Null};
    std::string str;
    double num{0.0};
    std::vector<JsonItem> arr;
    std::unordered_map<std::string, JsonItem> obj;

    const JsonItem* get(const std::string& k) const {
        auto it = obj.find(k);
        return (it != obj.end()) ? &it->second : nullptr;
    }
};

JsonItem parseItem(DispatcherJsonScanner& scan, JsonTok tok) {
    JsonItem item;
    if (tok.type == JsonTok::StrVal) { item.type = JsonItem::String; item.str = tok.str; return item; }
    if (tok.type == JsonTok::NumVal) { item.type = JsonItem::Number; item.num = tok.num; return item; }
    if (tok.type == JsonTok::BoolVal) { item.type = JsonItem::String; item.str = tok.str; return item; }
    if (tok.type == JsonTok::ArrOpen) {
        item.type = JsonItem::Array;
        while (true) {
            JsonTok t = scan.next();
            if (t.type == JsonTok::ArrClose || t.type == JsonTok::Eof) break;
            if (t.type == JsonTok::Comma) continue;
            item.arr.push_back(parseItem(scan, t));
        }
        return item;
    }
    if (tok.type == JsonTok::ObjOpen) {
        item.type = JsonItem::Object;
        while (true) {
            JsonTok k = scan.next();
            if (k.type == JsonTok::ObjClose || k.type == JsonTok::Eof) break;
            if (k.type == JsonTok::Comma) continue;
            if (k.type != JsonTok::StrVal) break;

            JsonTok col = scan.next();
            if (col.type != JsonTok::Colon) break;

            JsonTok v = scan.next();
            item.obj[k.str] = parseItem(scan, v);
        }
        return item;
    }
    return item;
}

} // anonymous namespace

bool EmergencyDispatcher::loadAmbulancesFromJson(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::stringstream buf;
    buf << file.rdbuf();
    std::string content = buf.str();
    if (content.empty()) return false;

    DispatcherJsonScanner scanner(std::move(content));
    JsonTok root_tok = scanner.next();
    if (root_tok.type != JsonTok::ArrOpen) return false;

    JsonItem root = parseItem(scanner, root_tok);
    clearAmbulances();

    for (const auto& item : root.arr) {
        if (item.type != JsonItem::Object) continue;
        const auto* id_node = item.get("id");
        const auto* loc_node = item.get("location_id");
        const auto* name_node = item.get("name");
        const auto* status_node = item.get("status");

        if (id_node && loc_node) {
            Ambulance amb;
            amb.id = id_node->str;
            amb.location_id = loc_node->str;
            amb.name = name_node ? name_node->str : amb.id;
            std::string st = status_node ? status_node->str : "Available";
            amb.is_available = (st == "Available" || st == "available" || st == "true");
            addAmbulance(amb);
        }
    }

    return !ambulances_.empty();
}

bool EmergencyDispatcher::loadHospitalsFromJson(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::stringstream buf;
    buf << file.rdbuf();
    std::string content = buf.str();
    if (content.empty()) return false;

    DispatcherJsonScanner scanner(std::move(content));
    JsonTok root_tok = scanner.next();
    if (root_tok.type != JsonTok::ArrOpen) return false;

    JsonItem root = parseItem(scanner, root_tok);
    clearHospitals();

    for (const auto& item : root.arr) {
        if (item.type != JsonItem::Object) continue;
        const auto* id_node = item.get("id");
        const auto* name_node = item.get("name");
        const auto* loc_node = item.get("location_id");

        if (id_node && name_node && loc_node) {
            Hospital hosp;
            hosp.id = id_node->str;
            hosp.name = name_node->str;
            hosp.location_id = loc_node->str;
            addHospital(hosp);
        }
    }

    return !hospitals_.empty();
}

} // namespace smartresq
