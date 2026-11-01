"""
SmartResQ - Backend Service (Python Flask API)
Connects the Leaflet.js Frontend with the High-Performance C++17 DSA Graph Engine.

Architecture:
Frontend (HTML/JS) <--> Flask REST API <--> C++ Engine (Subprocess IPC / JSON)

Core Endpoints:
- GET  /api/health            : Engine and dataset health check
- GET  /api/city              : Simulated city locations and roads (with blockage state)
- GET  /api/ambulances        : Ambulance fleet locations & availability
- GET  /api/hospitals         : Hospital facilities & emergency capacities
- POST /api/route             : Shortest path routing via C++ Dijkstra or BFS
- POST /api/ambulance/select  : Nearest ambulance dispatch via C++ Dijkstra
- POST /api/hospital/select   : Nearest hospital & ranked alternatives via C++ Dijkstra
- POST /api/road/block        : Dynamic road closure simulation
- POST /api/road/reopen       : Road reopening & baseline restoration
- POST /api/reset             : Reset graph blockages and fleet availability
"""

import os
import sys
import json
import threading
import subprocess
from pathlib import Path
from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS

BASE_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = BASE_DIR / "data"
FRONTEND_DIR = BASE_DIR / "frontend"

# Locate C++ Engine Executable
candidate_bins = [
    BASE_DIR / "cpp_engine" / "build" / ("smartresq_engine.exe" if os.name == "nt" else "smartresq_engine"),
    BASE_DIR / "cpp_engine" / ("smartresq_engine.exe" if os.name == "nt" else "smartresq_engine"),
    BASE_DIR / "build" / ("smartresq_engine.exe" if os.name == "nt" else "smartresq_engine"),
]
CPP_ENGINE_BIN = next((p for p in candidate_bins if p.exists()), candidate_bins[0])

app = Flask(__name__, static_folder=str(FRONTEND_DIR))
CORS(app)

# In-memory simulation state (persists road blockages and single-dispatch allocated ambulances)
simulation_state = {
    "blocked_roads": set(),
    "allocated_ambulances": set()
}
state_lock = threading.Lock()
simulation_operation_lock = threading.Lock()


def load_json_file(filename: str):
    """Safely loads JSON data from the project data directory."""
    filepath = DATA_DIR / filename
    if not filepath.exists():
        return None
    with open(filepath, "r", encoding="utf-8") as f:
        return json.load(f)


def engine_environment() -> dict:
    """Include the compiler runtime directory when the engine was built with MSYS2."""
    env = os.environ.copy()
    runtime_dirs = []

    configured_runtime = env.get("SMARTRESQ_ENGINE_RUNTIME_DIR")
    if configured_runtime:
        runtime_dirs.append(Path(configured_runtime))

    cache_file = CPP_ENGINE_BIN.parent / "CMakeCache.txt"
    if cache_file.exists():
        for line in cache_file.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith("CMAKE_CXX_COMPILER:") and "=" in line:
                compiler_path = Path(line.split("=", 1)[1])
                if compiler_path.is_absolute():
                    runtime_dirs.append(compiler_path.parent)
                break

    existing_path = env.get("PATH", "")
    path_entries = [str(path) for path in runtime_dirs if path.is_dir()]
    if path_entries:
        env["PATH"] = os.pathsep.join(path_entries + [existing_path])
    return env


def invoke_cpp_engine(payload: dict) -> tuple[dict, int]:
    """
    Executes the C++ DSA engine as a subprocess with JSON-based standard I/O.
    All Dijkstra, BFS, selection, and blockage calculations run natively in C++.
    """
    if not CPP_ENGINE_BIN.exists():
        return {
            "status": "error",
            "error": "Routing engine is temporarily unavailable.",
            "debug": f"Engine binary not found at '{CPP_ENGINE_BIN}'. Build cpp_engine with CMake first.",
            "code": 500
        }, 500

    # Thread-safe retrieval of current simulation state
    with state_lock:
        payload.setdefault("blocked_roads", sorted(list(simulation_state["blocked_roads"])))
        payload.setdefault("allocated_ambulances", sorted(list(simulation_state["allocated_ambulances"])))

    json_input = json.dumps(payload)

    try:
        proc = subprocess.run(
            [str(CPP_ENGINE_BIN), "--json"],
            input=json_input,
            text=True,
            capture_output=True,
            cwd=str(BASE_DIR),
            timeout=10,
            env=engine_environment()
        )
    except subprocess.TimeoutExpired:
        app.logger.error("C++ engine timed out after 10 seconds for action %r", payload.get("action"))
        return {
            "status": "error",
            "error": "Routing engine is temporarily unavailable.",
            "debug": "C++ engine execution timed out after 10 seconds.",
            "code": 504
        }, 504
    except OSError as exc:
        app.logger.exception("Failed to start C++ engine")
        return {
            "status": "error",
            "error": "Routing engine is temporarily unavailable.",
            "debug": f"Failed to start C++ engine: {exc}",
            "code": 500
        }, 500

    if proc.returncode != 0:
        debug_info = {
            "returncode": proc.returncode,
            "stderr": proc.stderr.strip()[-2000:],
            "stdout": proc.stdout.strip()[-1000:]
        }
        app.logger.error("C++ engine exited with code %s: %s", proc.returncode, debug_info)
        return {
            "status": "error",
            "error": "Routing engine is temporarily unavailable.",
            "debug": debug_info,
            "code": 500
        }, 500

    try:
        result_data = json.loads(proc.stdout)
    except json.JSONDecodeError as decode_err:
        app.logger.error("C++ engine returned invalid JSON: %s", decode_err)
        return {
            "status": "error",
            "error": "Routing engine returned an invalid response.",
            "debug": {
                "parse_error": str(decode_err),
                "stdout": proc.stdout.strip()[-1000:],
                "stderr": proc.stderr.strip()[-1000:]
            },
            "code": 500
        }, 500

    if not isinstance(result_data, dict):
        app.logger.error("C++ engine returned JSON with unexpected type %s", type(result_data).__name__)
        return {
            "status": "error",
            "error": "Routing engine returned an invalid response.",
            "debug": {"response_type": type(result_data).__name__},
            "code": 500
        }, 500

    if result_data.get("status") == "error":
        try:
            http_code = int(result_data.get("code", 400))
        except (TypeError, ValueError):
            http_code = 400
        if not 400 <= http_code <= 599:
            http_code = 400
        if proc.stderr.strip():
            result_data.setdefault("debug", proc.stderr.strip()[-2000:])
            app.logger.error("C++ engine reported an error: %s", proc.stderr.strip()[-2000:])
        return result_data, http_code

    return result_data, 200


# ============================================================================
# REST API ENDPOINTS
# ============================================================================

@app.route("/api/health", methods=["GET"])
def health_check():
    """Health check endpoint to verify backend, C++ binary, and datasets."""
    with state_lock:
        blocked = sorted(list(simulation_state["blocked_roads"]))
        allocated = sorted(list(simulation_state["allocated_ambulances"]))

    engine_result, engine_status = invoke_cpp_engine({"action": "display_graph"})
    engine_available = (
        engine_status == 200
        and engine_result.get("status") == "success"
        and isinstance(engine_result.get("node_count"), (int, float))
        and engine_result.get("node_count") > 0
    )

    return jsonify({
        "status": "healthy",
        "service": "SmartResQ Emergency Dispatch Engine",
        "version": "1.0.0",
        "cpp_engine": {
            "available": engine_available,
            "binary_path": str(CPP_ENGINE_BIN),
            "message": None if engine_available else "Routing engine is temporarily unavailable."
        },
        "simulation_state": {
            "blocked_roads": blocked,
            "blocked_count": len(blocked),
            "allocated_ambulances": allocated,
            "allocated_count": len(allocated)
        },
        "dataset_status": {
            "city_graph_available": (DATA_DIR / "city_graph.json").exists(),
            "ambulances_available": (DATA_DIR / "ambulances.json").exists(),
            "hospitals_available": (DATA_DIR / "hospitals.json").exists()
        }
    }), 200


@app.route("/api/city", methods=["GET"])
def get_city():
    """Returns simulated city locations and roads with active blockage status."""
    graph_data = load_json_file("city_graph.json")
    if not graph_data:
        return jsonify({"status": "error", "error": "city_graph.json not found"}), 404

    with state_lock:
        blocked = set(simulation_state["blocked_roads"])

    # Augment roads with dynamic blockage flag
    roads = []
    for road in graph_data.get("roads", []):
        road_copy = dict(road)
        road_copy["is_blocked"] = road["id"] in blocked
        roads.append(road_copy)

    return jsonify({
        "status": "success",
        "city_name": graph_data.get("city_name", "Metropolis Central"),
        "locations": graph_data.get("locations", []),
        "roads": roads,
        "summary": {
            "total_locations": len(graph_data.get("locations", [])),
            "total_roads": len(roads),
            "blocked_roads": sorted(list(blocked))
        }
    }), 200


@app.route("/api/ambulances", methods=["GET"])
def get_ambulances():
    """Returns ambulance fleet with active allocation status."""
    data = load_json_file("ambulances.json")
    if not data:
        return jsonify({"status": "error", "error": "ambulances.json not found"}), 404

    with state_lock:
        allocated = set(simulation_state["allocated_ambulances"])

    ambulances = []
    for amb in data:
        amb_copy = dict(amb)
        is_allocated = amb["id"] in allocated
        amb_copy["is_available"] = not is_allocated
        amb_copy["status"] = "Allocated" if is_allocated else "Available"
        ambulances.append(amb_copy)

    return jsonify({
        "status": "success",
        "ambulances": ambulances,
        "allocated_ambulances": sorted(list(allocated))
    }), 200


@app.route("/api/hospitals", methods=["GET"])
def get_hospitals():
    """Returns hospital facilities and trauma center capacities."""
    data = load_json_file("hospitals.json")
    if not data:
        return jsonify({"status": "error", "error": "hospitals.json not found"}), 404

    return jsonify({
        "status": "success",
        "hospitals": data
    }), 200


@app.route("/api/route", methods=["POST"])
def calculate_route():
    """
    Computes shortest route between two locations using native C++ Dijkstra or BFS.
    Respects active road blockages.
    """
    req_data = request.get_json(silent=True)
    if not req_data or not isinstance(req_data, dict):
        return jsonify({"status": "error", "error": "Invalid or missing JSON request body."}), 400

    origin = req_data.get("origin")
    destination = req_data.get("destination")
    algorithm = req_data.get("algorithm", "dijkstra")

    if not origin or not isinstance(origin, str) or not origin.strip():
        return jsonify({"status": "error", "error": "Missing required parameter 'origin'."}), 400
    if not destination or not isinstance(destination, str) or not destination.strip():
        return jsonify({"status": "error", "error": "Missing required parameter 'destination'."}), 400

    payload = {
        "action": "route",
        "origin": origin.strip(),
        "destination": destination.strip(),
        "algorithm": algorithm.strip().lower() if isinstance(algorithm, str) else "dijkstra"
    }

    result, status_code = invoke_cpp_engine(payload)
    return jsonify(result), status_code


@app.route("/api/ambulance/select", methods=["POST"])
def select_ambulance():
    """
    Selects the nearest available ambulance to an incident location using native C++ Dijkstra.
    Marks the chosen ambulance as allocated to prevent double dispatch.
    """
    req_data = request.get_json(silent=True)
    if not req_data or not isinstance(req_data, dict):
        return jsonify({"status": "error", "error": "Invalid or missing JSON request body."}), 400

    incident_id = req_data.get("incident_location_id") or req_data.get("incident_location")
    if not incident_id or not isinstance(incident_id, str) or not incident_id.strip():
        return jsonify({"status": "error", "error": "Missing required parameter 'incident_location_id'."}), 400

    payload = {
        "action": "ambulance_select",
        "incident_location_id": incident_id.strip()
    }

    with simulation_operation_lock:
        result, status_code = invoke_cpp_engine(payload)

        # Serialize dispatches so concurrent requests cannot allocate the same unit.
        if status_code == 200 and result.get("success") and "selected_ambulance" in result:
            selected_id = result["selected_ambulance"].get("id")
            if selected_id:
                with state_lock:
                    simulation_state["allocated_ambulances"].add(selected_id)
                    result["allocated_ambulances"] = sorted(list(simulation_state["allocated_ambulances"]))

    return jsonify(result), status_code


@app.route("/api/hospital/select", methods=["POST"])
def select_hospital():
    """
    Selects the nearest reachable hospital from a patient location using native C++ Dijkstra.
    Returns primary choice and ranked alternative facilities.
    """
    req_data = request.get_json(silent=True)
    if not req_data or not isinstance(req_data, dict):
        return jsonify({"status": "error", "error": "Invalid or missing JSON request body."}), 400

    patient_id = req_data.get("patient_location_id") or req_data.get("incident_location_id")
    if not patient_id or not isinstance(patient_id, str) or not patient_id.strip():
        return jsonify({"status": "error", "error": "Missing required parameter 'patient_location_id'."}), 400

    payload = {
        "action": "hospital_select",
        "patient_location_id": patient_id.strip()
    }

    result, status_code = invoke_cpp_engine(payload)
    return jsonify(result), status_code


@app.route("/api/road/block", methods=["POST"])
def block_road():
    """
    Blocks a road in the city graph using native C++.
    Subsequent route calculations will route around this blockage.
    """
    req_data = request.get_json(silent=True)
    if not req_data or not isinstance(req_data, dict):
        return jsonify({"status": "error", "error": "Invalid or missing JSON request body."}), 400

    road_id = req_data.get("road_id")
    if not road_id or not isinstance(road_id, str) or not road_id.strip():
        return jsonify({"status": "error", "error": "Missing required parameter 'road_id'."}), 400

    payload = {
        "action": "road_block",
        "road_id": road_id.strip()
    }

    with simulation_operation_lock:
        result, status_code = invoke_cpp_engine(payload)
        if status_code == 200 and result.get("status") == "success":
            with state_lock:
                simulation_state["blocked_roads"].add(road_id.strip())
                result["all_blocked_roads"] = sorted(list(simulation_state["blocked_roads"]))

    return jsonify(result), status_code


@app.route("/api/road/reopen", methods=["POST"])
def reopen_road():
    """
    Reopens a previously blocked road in the city graph using native C++.
    """
    req_data = request.get_json(silent=True)
    if not req_data or not isinstance(req_data, dict):
        return jsonify({"status": "error", "error": "Invalid or missing JSON request body."}), 400

    road_id = req_data.get("road_id")
    if not road_id or not isinstance(road_id, str) or not road_id.strip():
        return jsonify({"status": "error", "error": "Missing required parameter 'road_id'."}), 400

    payload = {
        "action": "road_reopen",
        "road_id": road_id.strip()
    }

    with simulation_operation_lock:
        result, status_code = invoke_cpp_engine(payload)
        if status_code == 200 and result.get("status") == "success":
            with state_lock:
                simulation_state["blocked_roads"].discard(road_id.strip())
                result["all_blocked_roads"] = sorted(list(simulation_state["blocked_roads"]))

    return jsonify(result), status_code


@app.route("/api/reset", methods=["POST"])
def reset_simulation():
    """
    Resets all road blockages and clears ambulance allocations back to original state.
    """
    payload = {"action": "reset"}
    with simulation_operation_lock:
        result, status_code = invoke_cpp_engine(payload)

        if status_code == 200 and result.get("status") == "success":
            with state_lock:
                simulation_state["blocked_roads"].clear()
                simulation_state["allocated_ambulances"].clear()
            result["all_blocked_roads"] = []
            result["allocated_ambulances"] = []
    return jsonify(result), status_code


# Backwards compatibility / convenience endpoint for existing frontend
@app.route("/api/city-data", methods=["GET"])
def get_city_data():
    """Legacy bundle endpoint providing city, ambulances, and hospitals."""
    graph_res, _ = get_city()
    amb_res, _ = get_ambulances()
    hosp_res, _ = get_hospitals()

    return jsonify({
        "city": graph_res.get_json(),
        "ambulances": amb_res.get_json().get("ambulances", []),
        "hospitals": hosp_res.get_json().get("hospitals", [])
    }), 200


# Frontend static files routing
@app.route("/")
def serve_index():
    return send_from_directory(str(FRONTEND_DIR), "index.html")


@app.route("/<path:path>")
def serve_static(path):
    if (FRONTEND_DIR / path).exists():
        return send_from_directory(str(FRONTEND_DIR), path)
    return send_from_directory(str(FRONTEND_DIR), "index.html")


@app.after_request
def add_cache_control_headers(response):
    """Prevents browsers from caching static JS/CSS files so updates load immediately."""
    response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate, max-age=0"
    response.headers["Pragma"] = "no-cache"
    response.headers["Expires"] = "0"
    return response


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    print(f"================================================================")
    print(f" SmartResQ Flask Backend Active on http://127.0.0.1:{port}")
    print(f" C++ Engine Binary : {CPP_ENGINE_BIN}")
    print(f" C++ Binary Status : {'EXISTS' if CPP_ENGINE_BIN.exists() else 'NOT FOUND'}")
    print(f"================================================================")
    app.run(host="127.0.0.1", port=port, debug=True)
