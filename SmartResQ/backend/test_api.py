"""
SmartResQ - Automated Integration Test Suite for Flask Backend & C++ Engine Bridge.
Tests all 10 endpoints, Dijkstra/BFS routing, dynamic road blockages, and ambulance allocation state.
"""

import sys
import json
from pathlib import Path

# Add backend directory to path
BACKEND_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(BACKEND_DIR))

from app import app, simulation_state


def run_all_tests():
    client = app.test_client()
    passed = 0
    total = 0

    def assert_test(condition, test_name, details=""):
        nonlocal passed, total
        total += 1
        if condition:
            passed += 1
            print(f" [PASS] Test {total:02d}: {test_name}")
            if details:
                print(f"        -> {details}")
        else:
            print(f" [FAIL] Test {total:02d}: {test_name}")
            if details:
                print(f"        -> Error details: {details}")

    print("====================================================================")
    print("   SmartResQ - Flask API & C++ Engine Integration Test Suite         ")
    print("====================================================================\n")

    # 1. Health check endpoint
    res = client.get("/api/health")
    data = res.get_json()
    assert_test(
        res.status_code == 200 and data.get("status") == "healthy" and data.get("cpp_engine", {}).get("available"),
        "GET /api/health",
        f"Service healthy, C++ engine verified available at '{data.get('cpp_engine', {}).get('binary_path')}'"
    )

    # 2. City graph endpoint
    res = client.get("/api/city")
    data = res.get_json()
    assert_test(
        res.status_code == 200 and len(data.get("locations", [])) == 18 and len(data.get("roads", [])) == 29,
        "GET /api/city",
        f"Loaded {len(data.get('locations', []))} locations and {len(data.get('roads', []))} roads"
    )

    # 3. Ambulances endpoint
    res = client.get("/api/ambulances")
    data = res.get_json()
    all_avail = all(a.get("is_available") for a in data.get("ambulances", []))
    assert_test(
        res.status_code == 200 and len(data.get("ambulances", [])) == 3 and all_avail,
        "GET /api/ambulances",
        f"{len(data.get('ambulances', []))} ambulances loaded, all initially available"
    )

    # 4. Hospitals endpoint
    res = client.get("/api/hospitals")
    data = res.get_json()
    assert_test(
        res.status_code == 200 and len(data.get("hospitals", [])) == 3,
        "GET /api/hospitals",
        f"{len(data.get('hospitals', []))} trauma hospitals loaded"
    )

    # 5. Dijkstra Route calculation (L1 -> L8)
    res = client.post("/api/route", json={"origin": "L1", "destination": "L8", "algorithm": "dijkstra"})
    data = res.get_json()
    assert_test(
        res.status_code == 200 and data.get("found") is True and round(data.get("distance_km", 0), 2) == 4.60,
        "POST /api/route (Dijkstra Shortest Path)",
        f"Route L1 -> L8: {data.get('distance_km')} km via path {data.get('path_nodes')}"
    )

    # 6. BFS Route calculation (L1 -> L8)
    res = client.post("/api/route", json={"origin": "L1", "destination": "L8", "algorithm": "bfs"})
    data = res.get_json()
    assert_test(
        res.status_code == 200 and data.get("found") is True and data.get("hop_count") == 3,
        "POST /api/route (BFS Minimum-Hop Route)",
        f"BFS Route found with {data.get('hop_count')} hops: {data.get('path_nodes')}"
    )

    # 7. Route error handling for invalid vertex
    res = client.post("/api/route", json={"origin": "L1", "destination": "INVALID_LOC"})
    data = res.get_json()
    assert_test(
        res.status_code == 400 and "error" in data,
        "POST /api/route (Invalid Location Error Handling)",
        f"Clean error returned: {data.get('error')}"
    )

    # 8. Route error handling for missing origin
    res = client.post("/api/route", json={"destination": "L2"})
    data = res.get_json()
    assert_test(
        res.status_code == 400 and "error" in data,
        "POST /api/route (Missing Parameter Error Handling)",
        f"Clean validation error: {data.get('error')}"
    )

    # 9. First Ambulance Selection (Nearest to L1 should be AMB-03 at L16, dist 1.1 km)
    res = client.post("/api/ambulance/select", json={"incident_location_id": "L1"})
    data = res.get_json()
    selected_amb1 = data.get("selected_ambulance", {}).get("id")
    assert_test(
        res.status_code == 200 and data.get("success") is True and selected_amb1 == "AMB-03",
        "POST /api/ambulance/select (First Dispatch - Nearest)",
        f"Selected {data.get('selected_ambulance', {}).get('name')} ({selected_amb1}) at {data.get('distance_km')} km"
    )

    # 10. Second Ambulance Selection (Prevents Double Dispatch: AMB-03 is busy, selects AMB-01)
    res = client.post("/api/ambulance/select", json={"incident_location_id": "L1"})
    data = res.get_json()
    selected_amb2 = data.get("selected_ambulance", {}).get("id")
    assert_test(
        res.status_code == 200 and data.get("success") is True and selected_amb2 == "AMB-01",
        "POST /api/ambulance/select (Double-Allocation Prevention)",
        f"AMB-03 busy. Selected next available: {selected_amb2} at {data.get('distance_km')} km"
    )

    # 11. Verify GET /api/ambulances reflects allocated units
    res = client.get("/api/ambulances")
    data = res.get_json()
    allocated_set = set(data.get("allocated_ambulances", []))
    assert_test(
        "AMB-03" in allocated_set and "AMB-01" in allocated_set,
        "GET /api/ambulances (State Reflection)",
        f"Allocated ambulances tracked: {allocated_set}"
    )

    # 12. Nearest Hospital Selection (for patient at L1: HOSP-01 at L2, dist 1.2 km)
    res = client.post("/api/hospital/select", json={"patient_location_id": "L1"})
    data = res.get_json()
    primary_hosp = data.get("selected_hospital", {}).get("id")
    alts = data.get("alternatives", [])
    assert_test(
        res.status_code == 200 and data.get("success") is True and primary_hosp == "HOSP-01" and len(alts) == 2,
        "POST /api/hospital/select (Hospital & Alternatives)",
        f"Primary: {data.get('selected_hospital', {}).get('name')}, Alternatives: {len(alts)} hospitals ranked"
    )

    # 13. Road Blockage (Block R01 between L1 and L2)
    res = client.post("/api/road/block", json={"road_id": "R01"})
    data = res.get_json()
    assert_test(
        res.status_code == 200 and data.get("status") == "success" and "R01" in data.get("all_blocked_roads", []),
        "POST /api/road/block (Simulate Incident on R01)",
        f"Road R01 blocked. Current blocked list: {data.get('all_blocked_roads')}"
    )

    # 14. GET /api/city reflects blocked road
    res = client.get("/api/city")
    data = res.get_json()
    r01_road = next((r for r in data.get("roads", []) if r["id"] == "R01"), None)
    assert_test(
        r01_road is not None and r01_road.get("is_blocked") is True,
        "GET /api/city (Road Blockage Reflection)",
        "Road R01 correctly has is_blocked: True in graph endpoint"
    )

    # 15. Dynamic Detour Recalculation after road blockage
    # Baseline was direct L1 -> L2 (1.2 km). With R01 blocked, detour is L1 -> L17 -> L2 (3.1 km)
    res = client.post("/api/route", json={"origin": "L1", "destination": "L2", "algorithm": "dijkstra"})
    data = res.get_json()
    detour_dist = data.get("distance_km", 0)
    detour_path = data.get("path_nodes", [])
    assert_test(
        res.status_code == 200 and data.get("found") is True and round(detour_dist, 2) == 3.10 and "L17" in detour_path,
        "POST /api/route (Dynamic Detour Recalculation)",
        f"Detour calculated via {detour_path} with new distance {detour_dist} km (+1.90 km delta)"
    )

    # 16. Road Reopening (Reopen R01)
    res = client.post("/api/road/reopen", json={"road_id": "R01"})
    data = res.get_json()
    assert_test(
        res.status_code == 200 and data.get("status") == "success" and "R01" not in data.get("all_blocked_roads", []),
        "POST /api/road/reopen (Clear Incident on R01)",
        f"Road R01 reopened. Active blocked roads: {data.get('all_blocked_roads')}"
    )

    # 17. Baseline route restoration after reopening
    res = client.post("/api/route", json={"origin": "L1", "destination": "L2", "algorithm": "dijkstra"})
    data = res.get_json()
    restored_dist = data.get("distance_km", 0)
    assert_test(
        res.status_code == 200 and round(restored_dist, 2) == 1.20 and data.get("path_nodes") == ["L1", "L2"],
        "POST /api/route (Route Baseline Restoration)",
        f"Optimal baseline path restored: {data.get('path_nodes')} ({restored_dist} km)"
    )

    # 18. Simulation Reset
    res = client.post("/api/reset")
    data = res.get_json()
    assert_test(
        res.status_code == 200 and data.get("status") == "success",
        "POST /api/reset (Graph & Fleet Reset)",
        "Reset confirmed by C++ engine and simulation state cleared"
    )

    # 19. Verify full reset across city and ambulances
    res_amb = client.get("/api/ambulances")
    data_amb = res_amb.get_json()
    all_free = all(a.get("is_available") for a in data_amb.get("ambulances", []))
    res_city = client.get("/api/city")
    data_city = res_city.get_json()
    none_blocked = all(not r.get("is_blocked") for r in data_city.get("roads", []))
    assert_test(
        all_free and none_blocked,
        "Verification After Reset",
        f"All ambulances available ({all_free}), all roads open ({none_blocked})"
    )

    print("\n====================================================================")
    print(f"  INTEGRATION TEST RESULTS: {passed}/{total} TESTS PASSED ({(passed/total)*100:.1f}%)")
    print("====================================================================")
    return passed == total


if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)

