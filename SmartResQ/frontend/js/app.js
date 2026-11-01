/**
 * SmartResQ - Emergency Ambulance Routing Simulation Frontend
 * Connects Leaflet.js visualization with Flask REST API & Native C++17 DSA Engine.
 */

const API_BASE = '/api';
const SIMULATED_SPEED_KMH = 40;

// Application State
const state = {
  city: null,
  ambulances: [],
  hospitals: [],
  selectedIncidentId: null,
  map: null,
  markers: {},
  roadLayers: new Map(), // roadId -> Leaflet Polyline
  routeLayers: [],       // Array of active route polylines
  mapFitted: false,
  blockedRoads: new Set(),
  allocatedAmbulances: new Set(),
  activeMission: null,
  roadsBusy: false,
  dijkstraRoute: null,
  bfsRoute: null,
  noticeTimeout: null
};

// Initialize Application on DOM Ready
document.addEventListener('DOMContentLoaded', () => {
  initMap();
  setupTabs();
  setupEventListeners();
  checkBackendHealth();
  fetchCityData();
});

// ============================================================================
// 1. LEAFLET MAP INITIALIZATION & CONTROLS
// ============================================================================

function initMap() {
  const defaultCenter = [18.525, 73.855];
  state.map = L.map('map', {
    zoomControl: true,
    attributionControl: false
  }).setView(defaultCenter, 13);

  // 1. Dark Navy OpenStreetMap Layer (100% Free, Zero API Key Required)
  const darkOSM = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    className: 'dark-tiles'
  });

  // 2. Standard Streets OpenStreetMap Layer (100% Free, Zero API Key Required)
  const standardOSM = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
  });

  // 3. Esri Satellite Imagery Layer (Free Public Access, Zero API Key Required)
  const esriSatellite = L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
    maxZoom: 19,
    attribution: '&copy; Esri, Earthstar Geographics'
  });

  // Default to Dark Navy Tiles
  darkOSM.addTo(state.map);

  // Layer control allowing user to switch basemaps freely
  const baseMaps = {
    "🌙 Dark Navy (Free, No Key)": darkOSM,
    "🗺️ Standard Streets (OSM)": standardOSM,
    "🛰️ Satellite Imagery (Esri)": esriSatellite
  };
  L.control.layers(baseMaps, null, { position: 'topleft' }).addTo(state.map);
}

function recenterMap() {
  fitCityBounds();
}

function fitCityBounds() {
  if (!state.map || !state.city?.locations?.length) return;
  const locations = state.city.locations
    .filter(loc => Number.isFinite(loc.latitude) && Number.isFinite(loc.longitude))
    .map(loc => [loc.latitude, loc.longitude]);
  if (locations.length) {
    state.map.fitBounds(locations, { padding: [36, 36], maxZoom: 14 });
  }
}

function clearRouteLayers() {
  state.routeLayers.forEach(l => state.map.removeLayer(l));
  state.routeLayers = [];
}

function showNotice(message, type = 'error') {
  const notice = document.getElementById('app-notice');
  if (!notice) return;
  window.clearTimeout(state.noticeTimeout);
  notice.textContent = message;
  notice.className = `app-notice app-notice-${type}`;
  state.noticeTimeout = window.setTimeout(() => notice.classList.add('hidden'), 5000);
}

function friendlyError(data, fallback = 'The request could not be completed. Please try again.') {
  const message = String(data?.error || data?.message || '').toLowerCase();
  if (message.includes('not found') || message.includes('invalid')) {
    return 'The selected location or road is invalid.';
  }
  if (message.includes('no available') || message.includes('ambulance')) {
    return 'No ambulance is currently available for this incident.';
  }
  if (message.includes('engine') || message.includes('cpp') || message.includes('temporarily unavailable')) {
    return 'Routing engine is temporarily unavailable. Please try again shortly.';
  }
  return fallback;
}

async function readJsonResponse(response) {
  try {
    return await response.json();
  } catch {
    throw new Error('The server returned an invalid response.');
  }
}

// ============================================================================
// 2. TAB NAVIGATION
// ============================================================================

function setupTabs() {
  const tabBtns = document.querySelectorAll('.tab-btn');
  const tabPanes = document.querySelectorAll('.tab-pane');

  tabBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      const targetTab = btn.getAttribute('data-tab');

      tabBtns.forEach(b => b.classList.remove('active'));
      tabPanes.forEach(p => p.classList.remove('active'));

      btn.classList.add('active');
      const activePane = document.getElementById(targetTab);
      if (activePane) activePane.classList.add('active');
    });
  });
}

// ============================================================================
// 3. BACKEND API INTEGRATION & DATA FETCHING
// ============================================================================

async function checkBackendHealth() {
  const statusLabel = document.querySelector('.status-label');
  const statusDot = document.querySelector('.status-dot');

  try {
    const res = await fetch(`${API_BASE}/health`);
    if (res.ok) {
      const data = await res.json();
      if (data.cpp_engine && data.cpp_engine.available) {
        statusLabel.textContent = 'C++ DSA Engine Online';
        statusDot.style.backgroundColor = '#10b981';
      } else {
        statusLabel.textContent = 'Backend Online (Engine Missing)';
        statusDot.style.backgroundColor = '#f59e0b';
      }
      updateCounters(data.simulation_state);
    } else {
      throw new Error(`HTTP ${res.status}`);
    }
  } catch (err) {
    console.warn('Backend currently offline or unreachable:', err);
    statusLabel.textContent = 'Backend Offline';
    statusDot.style.backgroundColor = '#ef4444';
  }
}

async function fetchCityData() {
  try {
    const [cityRes, ambRes, hospRes] = await Promise.all([
      fetch(`${API_BASE}/city`),
      fetch(`${API_BASE}/ambulances`),
      fetch(`${API_BASE}/hospitals`)
    ]);

    if (!cityRes.ok || !ambRes.ok || !hospRes.ok) {
      throw new Error('Failed to retrieve city data from API');
    }

    state.city = await cityRes.json();
    const ambData = await ambRes.json();
    const hospData = await hospRes.json();

    state.ambulances = ambData.ambulances || [];
    state.hospitals = hospData.hospitals || [];

    // Sync blocked roads
    const blockedList = state.city.summary ? state.city.summary.blocked_roads : [];
    state.blockedRoads = new Set(blockedList);

    // Sync allocated ambulances
    state.allocatedAmbulances = new Set(ambData.allocated_ambulances || []);

    updateCounters({
      blocked_count: state.blockedRoads.size,
      allocated_count: state.allocatedAmbulances.size
    });

    renderCityGraph();
    populateAllDropdowns();
    renderActiveBlockedRoads();
  } catch (err) {
    console.error('Error loading city data:', err);
    showNotice('City network data could not be loaded. Check that the backend is running.');
  }
}

function updateCounters(simState) {
  if (!simState) return;
  const blockedLabel = document.getElementById('blocked-count-label');
  const dispatchedLabel = document.getElementById('dispatched-count-label');

  const bCount = simState.blocked_count !== undefined ? simState.blocked_count : simState.blocked_roads?.length || 0;
  const aCount = simState.allocated_count !== undefined ? simState.allocated_count : simState.allocated_ambulances?.length || 0;

  if (blockedLabel) blockedLabel.textContent = `${bCount} Blocked Roads`;
  if (dispatchedLabel) dispatchedLabel.textContent = `${aCount} Dispatched Units`;

  const activeBlockedBadge = document.getElementById('active-blocked-badge');
  if (activeBlockedBadge) activeBlockedBadge.textContent = `${bCount} Active`;
}

// ============================================================================
// 4. MAP RENDERING (NODES, ROADS & STATUS)
// ============================================================================

function renderCityGraph() {
  if (!state.city || !state.city.locations) return;

  const locationMap = new Map();
  state.city.locations.forEach(loc => locationMap.set(loc.id, loc));

  // 1. Clear existing road layers
  state.roadLayers.forEach(layer => state.map.removeLayer(layer));
  state.roadLayers.clear();

  // 2. Render road polylines
  state.city.roads.forEach(road => {
    const src = locationMap.get(road.source);
    const dest = locationMap.get(road.destination);

    if (src && dest) {
      const isBlocked = road.is_blocked || state.blockedRoads.has(road.id);
      const latlngs = [
        [src.latitude, src.longitude],
        [dest.latitude, dest.longitude]
      ];

      const line = L.polyline(latlngs, {
        color: isBlocked ? '#ef4444' : '#334155',
        weight: isBlocked ? 5 : 3,
        opacity: isBlocked ? 0.95 : 0.65,
        dashArray: isBlocked ? '6, 6' : null
      }).addTo(state.map);

      const statusTag = isBlocked ? '🛑 <strong>BLOCKED</strong> (Click to Reopen)' : '🟢 <strong>OPEN</strong> (Click to Block)';
      line.bindTooltip(`Road <strong>${road.id}</strong>: ${road.distance_km} km<br>${src.name} ↔ ${dest.name}<br><small>${statusTag}</small>`, {
        sticky: true,
        className: 'road-tooltip'
      });

      // Click to toggle blockage directly on map
      line.on('click', () => {
        if (isBlocked) {
          reopenRoad(road.id);
        } else {
          blockRoad(road.id);
        }
      });

      state.roadLayers.set(road.id, line);
    }
  });

  // 3. Render location markers
  const ambulanceLocations = new Set(state.ambulances.map(a => a.location_id));
  const hospitalLocations = new Set(state.hospitals.map(h => h.location_id));

  state.city.locations.forEach(loc => {
    if (state.markers[loc.id]) {
      state.map.removeLayer(state.markers[loc.id]);
    }

    let markerColor = '#475569'; // default city landmark
    let markerBorder = '#94a3b8';
    let markerRadius = 7;
    let label = `<strong>${loc.name}</strong> (${loc.id})`;

    if (loc.id === state.selectedIncidentId) {
      markerColor = '#ef4444';
      markerBorder = '#fecaca';
      markerRadius = 12;
      label = `🚨 <strong>EMERGENCY INCIDENT</strong><br>${loc.name} (${loc.id})`;
    } else if (hospitalLocations.has(loc.id)) {
      markerColor = '#10b981';
      markerBorder = '#a7f3d0';
      markerRadius = 10;
      const hosp = state.hospitals.find(h => h.location_id === loc.id);
      label = `🏥 <strong>${hosp ? hosp.name : 'Hospital'}</strong> (${loc.id})<br>Trauma Center | ICU Beds: ${hosp ? hosp.available_icu_beds : 0}`;
    } else if (ambulanceLocations.has(loc.id)) {
      markerColor = '#3b82f6';
      markerBorder = '#bfdbfe';
      markerRadius = 9;
      const amb = state.ambulances.find(a => a.location_id === loc.id);
      const isAllocated = amb && (!amb.is_available || state.allocatedAmbulances.has(amb.id));
      const statusText = isAllocated ? 'DISPATCHED (In Mission)' : 'AVAILABLE';
      label = `🚑 <strong>${amb ? amb.name : 'Ambulance'}</strong> (${loc.id})<br>Station: ${loc.name}<br>Status: <strong>${statusText}</strong>`;
      if (isAllocated) {
        markerColor = '#6366f1';
      }
    }

    const marker = L.circleMarker([loc.latitude, loc.longitude], {
      radius: markerRadius,
      fillColor: markerColor,
      color: markerBorder,
      weight: 2,
      opacity: 1,
      fillOpacity: 0.95
    }).addTo(state.map);

    marker.bindPopup(label);
    marker.on('click', () => selectEmergencyLocation(loc.id));

    state.markers[loc.id] = marker;
  });

  if (!state.mapFitted) {
    fitCityBounds();
    state.mapFitted = true;
  }
  renderRoadTable();
}

// ============================================================================
// 5. DROPDOWNS POPULATION
// ============================================================================

function populateAllDropdowns() {
  if (!state.city || !state.city.locations) return;

  const locs = state.city.locations;
  const roads = state.city.roads;

  // 1. Incident Select
  const incidentSelect = document.getElementById('incident-select');
  populateSelect(incidentSelect, locs.map(l => ({ value: l.id, text: `${l.name} (${l.id})` })), state.selectedIncidentId, 'Select emergency location...');

  // 2. Road Select
  const roadSelect = document.getElementById('road-select');
  populateSelect(roadSelect, roads.map(r => ({
    value: r.id,
    text: `Road ${r.id}: ${r.source} ↔ ${r.destination} (${r.distance_km} km)${r.is_blocked || state.blockedRoads.has(r.id) ? ' [BLOCKED]' : ' [OPEN]'}`
  })), null, 'Select road segment...');

  // 3. Detour Selectors
  const detourOrigin = document.getElementById('detour-origin-select');
  const detourDest = document.getElementById('detour-dest-select');
  populateSelect(detourOrigin, locs.map(l => ({ value: l.id, text: `${l.name} (${l.id})` })), 'L1');
  populateSelect(detourDest, locs.map(l => ({ value: l.id, text: `${l.name} (${l.id})` })), 'L2');

  // 4. Comparison Selectors
  const cmpOrigin = document.getElementById('compare-origin-select');
  const cmpDest = document.getElementById('compare-dest-select');
  populateSelect(cmpOrigin, locs.map(l => ({ value: l.id, text: `${l.name} (${l.id})` })), 'L1');
  populateSelect(cmpDest, locs.map(l => ({ value: l.id, text: `${l.name} (${l.id})` })), 'L8');
}

function renderRoadTable() {
  const body = document.getElementById('road-table-body');
  const count = document.getElementById('road-network-count');
  if (!body || !state.city?.roads) return;
  body.replaceChildren();
  if (count) count.textContent = `${state.city.roads.length} Roads`;

  state.city.roads.forEach(road => {
    const blocked = road.is_blocked || state.blockedRoads.has(road.id);
    const row = document.createElement('tr');
    const roadCell = document.createElement('td');
    roadCell.textContent = road.id;
    roadCell.className = 'road-id-cell';

    const locationsCell = document.createElement('td');
    locationsCell.textContent = `${road.source} ↔ ${road.destination}`;

    const distanceCell = document.createElement('td');
    distanceCell.textContent = `${Number(road.distance_km).toFixed(1)} km`;

    const statusCell = document.createElement('td');
    const action = document.createElement('button');
    action.type = 'button';
    action.className = `road-status-button ${blocked ? 'is-blocked' : 'is-open'}`;
    action.textContent = state.roadsBusy ? 'Updating…' : (blocked ? 'Blocked · Reopen' : 'Open · Block');
    action.disabled = state.roadsBusy;
    action.setAttribute('aria-label', `${blocked ? 'Reopen' : 'Block'} road ${road.id}`);
    action.addEventListener('click', () => {
      document.getElementById('road-select').value = road.id;
      (blocked ? reopenRoad : blockRoad)(road.id);
    });
    statusCell.appendChild(action);
    row.append(roadCell, locationsCell, distanceCell, statusCell);
    body.appendChild(row);
  });
}

function populateSelect(selectElem, items, selectedValue, placeholder) {
  if (!selectElem) return;
  const curr = selectedValue || selectElem.value;
  selectElem.innerHTML = '';

  if (placeholder) {
    const defaultOpt = document.createElement('option');
    defaultOpt.value = '';
    defaultOpt.disabled = true;
    defaultOpt.selected = !curr;
    defaultOpt.textContent = placeholder;
    selectElem.appendChild(defaultOpt);
  }

  items.forEach(item => {
    const opt = document.createElement('option');
    opt.value = item.value;
    opt.textContent = item.text;
    if (curr && item.value === curr) {
      opt.selected = true;
    }
    selectElem.appendChild(opt);
  });
}

// ============================================================================
// 6. EMERGENCY SELECTION & DISPATCH
// ============================================================================

function selectEmergencyLocation(locationId) {
  state.selectedIncidentId = locationId;
  state.activeMission = null;
  clearRouteLayers();
  ['amb-details', 'hosp-details', 'route-details'].forEach(id => {
    document.getElementById(id)?.classList.add('hidden');
  });
  ['amb-empty-state', 'hosp-empty-state', 'route-empty-state'].forEach(id => {
    document.getElementById(id)?.classList.remove('hidden');
  });
  document.getElementById('route-empty-state').textContent = 'Route trajectory and waypoints will appear after dispatch.';

  const incidentSelect = document.getElementById('incident-select');
  if (incidentSelect) incidentSelect.value = locationId;

  const dispatchBtn = document.getElementById('dispatch-btn');
  if (dispatchBtn) dispatchBtn.disabled = false;

  renderCityGraph();
}

async function executeEmergencyDispatch() {
  if (!state.selectedIncidentId) return;

  const dispatchBtn = document.getElementById('dispatch-btn');
  const dispatchLabel = dispatchBtn?.querySelector('span');
  const previousLabel = dispatchLabel?.textContent;
  if (dispatchBtn) dispatchBtn.disabled = true;
  if (dispatchLabel) dispatchLabel.textContent = 'Calculating routes…';

  try {
    clearRouteLayers();

    // 1. Dispatch Nearest Ambulance via C++ Dijkstra
    const ambRes = await fetch(`${API_BASE}/ambulance/select`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ incident_location_id: state.selectedIncidentId })
    });
    const ambData = await readJsonResponse(ambRes);

    if (!ambRes.ok || !ambData.success) {
      showNotice(friendlyError(ambData, 'No ambulance is currently available or reachable.'));
      return;
    }

    // 2. Select Nearest Hospital & Alternatives via C++ Dijkstra
    const hospRes = await fetch(`${API_BASE}/hospital/select`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ patient_location_id: state.selectedIncidentId })
    });
    const hospData = await readJsonResponse(hospRes);

    if (!hospRes.ok || !hospData.success) {
      showNotice(friendlyError(hospData, 'No hospital is reachable from this incident.'));
      return;
    }

    state.activeMission = {
      ambulance: ambData.selected_ambulance,
      hospital: hospData.selected_hospital
    };
    renderMission(ambData, hospData);

    // Refresh city & ambulance fleet data
    await fetchCityData();
  } catch (err) {
    console.error('Dispatch simulation error:', err);
    showNotice('Routing engine is temporarily unavailable. Please try again shortly.');
  } finally {
    if (dispatchBtn) dispatchBtn.disabled = false;
    if (dispatchLabel && previousLabel) dispatchLabel.textContent = previousLabel;
  }
}

function renderMission(ambData, hospData) {
  clearRouteLayers();
  drawPolylineRoute(
    ambData.path_nodes,
    '#f59e0b',
    6,
    null,
    `Leg 1 · Ambulance response: ${ambData.selected_ambulance.name} (${ambData.distance_km.toFixed(2)} km)`
  );
  drawPolylineRoute(
    hospData.path_nodes,
    '#06b6d4',
    6,
    '7, 5',
    `Leg 2 · Hospital transport: ${hospData.selected_hospital.name} (${hospData.distance_km.toFixed(2)} km)`
  );
  displayAmbulanceResult(ambData);
  displayHospitalResult(hospData);
  displayMissionRoute(ambData, hospData);
  if (state.city?.locations?.length) {
    const routeLocations = new Map(state.city.locations.map(loc => [loc.id, loc]));
    const routePoints = [...(ambData.path_nodes || []), ...(hospData.path_nodes || [])]
      .map(id => routeLocations.get(id))
      .filter(Boolean)
      .map(loc => [loc.latitude, loc.longitude]);
    if (routePoints.length) state.map.fitBounds(routePoints, { padding: [60, 60], maxZoom: 15 });
  }
}

async function refreshActiveMission() {
  if (!state.activeMission || !state.selectedIncidentId) return;
  try {
    const [ambulanceResponse, hospitalResponse] = await Promise.all([
      fetch(`${API_BASE}/route`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          origin: state.activeMission.ambulance.location_id,
          destination: state.selectedIncidentId,
          algorithm: 'dijkstra'
        })
      }),
      fetch(`${API_BASE}/hospital/select`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ patient_location_id: state.selectedIncidentId })
      })
    ]);
    const ambulanceRoute = await readJsonResponse(ambulanceResponse);
    const hospitalData = await readJsonResponse(hospitalResponse);
    if (!ambulanceResponse.ok || !ambulanceRoute.found) {
      clearRouteLayers();
      document.getElementById('route-details')?.classList.add('hidden');
      const emptyState = document.getElementById('route-empty-state');
      if (emptyState) {
        emptyState.textContent = 'No route is available because the connecting roads are blocked.';
        emptyState.classList.remove('hidden');
      }
      showNotice('No ambulance route is available because the connecting roads are blocked.');
      return;
    }
    if (!hospitalResponse.ok || !hospitalData.success) {
      clearRouteLayers();
      document.getElementById('route-details')?.classList.add('hidden');
      const emptyState = document.getElementById('route-empty-state');
      if (emptyState) {
        emptyState.textContent = 'No route is available because the connecting roads are blocked.';
        emptyState.classList.remove('hidden');
      }
      showNotice('No hospital route is available because the connecting roads are blocked.');
      return;
    }
    const ambulanceData = {
      ...ambulanceRoute,
      success: true,
      selected_ambulance: state.activeMission.ambulance
    };
    state.activeMission.hospital = hospitalData.selected_hospital;
    renderMission(ambulanceData, hospitalData);
  } catch (err) {
    console.error('Mission route refresh failed:', err);
    showNotice('Routes could not be refreshed. Please try again.');
  }
}

function displayAmbulanceResult(ambData) {
  const emptyState = document.getElementById('amb-empty-state');
  const details = document.getElementById('amb-details');
  const chip = document.getElementById('amb-status-chip');

  if (emptyState) emptyState.classList.add('hidden');
  if (details) details.classList.remove('hidden');

  const amb = ambData.selected_ambulance;
  document.getElementById('amb-name').textContent = amb.name;
  document.getElementById('amb-id').textContent = amb.id;
  document.getElementById('amb-station').textContent = `Station: ${amb.location_id}`;
  document.getElementById('amb-dist').textContent = `${ambData.distance_km.toFixed(2)} km`;
  document.getElementById('amb-eta').textContent = `${estimateMinutes(ambData.distance_km)} min`;

  if (chip) {
    chip.textContent = 'Dispatched';
    chip.className = 'chip chip-red';
  }
}

function displayHospitalResult(hospData) {
  const emptyState = document.getElementById('hosp-empty-state');
  const details = document.getElementById('hosp-details');

  if (emptyState) emptyState.classList.add('hidden');
  if (details) details.classList.remove('hidden');

  const hosp = hospData.selected_hospital;
  document.getElementById('hosp-name').textContent = hosp.name;
  document.getElementById('hosp-loc').textContent = hosp.location_id;
  document.getElementById('hosp-dist').textContent = `${hospData.distance_km.toFixed(2)} km`;

  // Render alternatives list
  const altsList = document.getElementById('hosp-alternatives-list');
  if (altsList) {
    altsList.innerHTML = '';
    if (hospData.alternatives && hospData.alternatives.length > 0) {
      hospData.alternatives.forEach(alt => {
        const item = document.createElement('div');
        item.className = 'alt-hospital-item';
        item.innerHTML = `
          <span class="alt-name">${alt.name} (${alt.location_id})</span>
          <span class="alt-distance">${alt.distance_km.toFixed(2)} km</span>
        `;
        altsList.appendChild(item);
      });
    } else {
      altsList.innerHTML = '<span class="empty-state">No other reachable hospitals</span>';
    }
  }
}

function displayMissionRoute(ambData, hospData) {
  const emptyState = document.getElementById('route-empty-state');
  const details = document.getElementById('route-details');

  if (emptyState) emptyState.classList.add('hidden');
  if (details) details.classList.remove('hidden');

  const leg1 = ambData.distance_km;
  const leg2 = hospData.distance_km;
  const total = (leg1 + leg2).toFixed(2);

  document.getElementById('route-leg1-km').textContent = `${leg1.toFixed(2)} km`;
  document.getElementById('route-leg2-km').textContent = `${leg2.toFixed(2)} km`;
  document.getElementById('route-total-km').textContent = `${total} km`;
  document.getElementById('route-eta').textContent = `${estimateMinutes(leg1)} min to incident`;

  renderWaypointsPills(
    document.getElementById('route-leg1-waypoints'),
    ambData.path_nodes,
    ambData.selected_ambulance.location_id,
    state.selectedIncidentId,
    null
  );
  renderWaypointsPills(
    document.getElementById('route-leg2-waypoints'),
    hospData.path_nodes,
    state.selectedIncidentId,
    null,
    hospData.selected_hospital.location_id
  );
}

function estimateMinutes(distanceKm) {
  return Math.max(1, Math.ceil((distanceKm / SIMULATED_SPEED_KMH) * 60));
}

function renderWaypointsPills(container, pathNodes, originId, incidentId, destId) {
  if (!container) return;
  container.innerHTML = '';

  pathNodes.forEach((node, idx) => {
    const pill = document.createElement('span');
    pill.className = 'node-pill';
    pill.textContent = node;

    if (node === originId) pill.classList.add('origin');
    else if (node === incidentId) pill.classList.add('incident');
    else if (node === destId) pill.classList.add('hospital');

    container.appendChild(pill);

    if (idx < pathNodes.length - 1) {
      const arrow = document.createElement('span');
      arrow.className = 'waypoint-arrow';
      arrow.textContent = '➔';
      container.appendChild(arrow);
    }
  });
}

function drawPolylineRoute(nodeIds, color, weight, dashArray, popupText) {
  if (!state.city || !nodeIds || nodeIds.length < 2) return;
  const locationMap = new Map();
  state.city.locations.forEach(loc => locationMap.set(loc.id, loc));

  const latlngs = [];
  nodeIds.forEach(id => {
    const loc = locationMap.get(id);
    if (loc) latlngs.push([loc.latitude, loc.longitude]);
  });

  if (latlngs.length >= 2) {
    const poly = L.polyline(latlngs, {
      color: color,
      weight: weight,
      opacity: 0.95,
      dashArray: dashArray,
      className: 'route-path'
    }).addTo(state.map);

    if (popupText) poly.bindTooltip(popupText, { sticky: true });
    state.routeLayers.push(poly);
  }
}

// ============================================================================
// 7. ROAD BLOCKAGE SIMULATION (PANEL 6)
// ============================================================================

async function blockRoad(roadId) {
  if (!roadId) return;
  setRoadsBusy(true);
  try {
    const res = await fetch(`${API_BASE}/road/block`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ road_id: roadId })
    });
    const data = await readJsonResponse(res);
    if (res.ok && data.status === 'success') {
      state.blockedRoads.add(roadId);
      await fetchCityData();
      showNotice(`Road ${roadId} blocked. Map and routes updated.`, 'success');
      await refreshActiveMission();
    } else {
      showNotice(friendlyError(data, 'The road could not be blocked.'));
    }
  } catch (err) {
    console.error('Error blocking road:', err);
    showNotice('The road could not be blocked. Check the backend connection.');
  } finally {
    setRoadsBusy(false);
  }
}

async function reopenRoad(roadId) {
  if (!roadId) return;
  setRoadsBusy(true);
  try {
    const res = await fetch(`${API_BASE}/road/reopen`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ road_id: roadId })
    });
    const data = await readJsonResponse(res);
    if (res.ok && data.status === 'success') {
      state.blockedRoads.delete(roadId);
      await fetchCityData();
      showNotice(`Road ${roadId} reopened. Map and routes updated.`, 'success');
      await refreshActiveMission();
    } else {
      showNotice(friendlyError(data, 'The road could not be reopened.'));
    }
  } catch (err) {
    console.error('Error reopening road:', err);
    showNotice('The road could not be reopened. Check the backend connection.');
  } finally {
    setRoadsBusy(false);
  }
}

function setRoadsBusy(busy) {
  state.roadsBusy = busy;
  ['btn-block-road', 'btn-reopen-road', 'road-select'].forEach(id => {
    const control = document.getElementById(id);
    if (control) control.disabled = busy;
  });
  document.querySelectorAll('.road-status-button, .reopen-pill-btn').forEach(button => {
    button.disabled = busy;
  });
  if (busy) {
    showNotice('Updating road status and recalculating the active routes…', 'success');
  } else {
    renderRoadTable();
  }
}

function renderActiveBlockedRoads() {
  const container = document.getElementById('active-blocked-list');
  const emptyMsg = document.getElementById('no-blocked-msg');
  if (!container) return;

  container.innerHTML = '';
  if (state.blockedRoads.size === 0) {
    if (emptyMsg) emptyMsg.classList.remove('hidden');
    return;
  }

  if (emptyMsg) emptyMsg.classList.add('hidden');

  state.blockedRoads.forEach(roadId => {
    const road = state.city?.roads?.find(item => item.id === roadId);
    const badge = document.createElement('div');
    badge.className = 'blocked-road-badge';
    const label = document.createElement('span');
    label.textContent = `🛑 ${roadId}${road ? ` · ${road.source} ↔ ${road.destination}` : ''}`;
    const reopenButton = document.createElement('button');
    reopenButton.className = 'reopen-pill-btn';
    reopenButton.type = 'button';
    reopenButton.title = `Reopen Road ${roadId}`;
    reopenButton.textContent = 'Reopen';
    reopenButton.disabled = state.roadsBusy;
    reopenButton.addEventListener('click', () => {
      reopenRoad(roadId);
    });
    badge.append(label, reopenButton);
    container.appendChild(badge);
  });
}

async function recalculateDetour() {
  const origin = document.getElementById('detour-origin-select')?.value;
  const dest = document.getElementById('detour-dest-select')?.value;

  if (!origin || !dest) {
    showNotice('Select both an origin and destination to calculate a detour.');
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/route`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ origin, destination: dest, algorithm: 'dijkstra' })
    });
    const data = await readJsonResponse(res);

    const box = document.getElementById('detour-result-box');
    if (!box) return;

    if (res.ok && data.found) {
      box.classList.remove('hidden');
      document.getElementById('detour-distance').textContent = `${data.distance_km.toFixed(2)} km`;
      document.getElementById('detour-hops').textContent = data.hop_count;
      document.getElementById('detour-status').textContent = 'Path Found';
      document.getElementById('detour-status').className = 'chip chip-green';

      renderWaypointsPills(document.getElementById('detour-waypoints'), data.path_nodes, origin, null, dest);

      // Draw detour on map in vibrant magenta
      clearRouteLayers();
      drawPolylineRoute(
        data.path_nodes,
        '#ec4899',
        5,
        null,
        `🔄 C++ Detour: ${origin} ➔ ${dest} (${data.distance_km.toFixed(2)} km, ${data.hop_count} hops)`
      );
    } else {
      box.classList.remove('hidden');
      document.getElementById('detour-distance').textContent = 'Unreachable';
      document.getElementById('detour-hops').textContent = '0';
      document.getElementById('detour-status').textContent = 'No Path Exists';
      document.getElementById('detour-status').className = 'chip chip-red';
      document.getElementById('detour-waypoints').textContent = 'No route is available because the connecting roads are blocked.';
      clearRouteLayers();
    }
  } catch (err) {
    console.error('Error recalculating detour:', err);
    showNotice('Routing engine is temporarily unavailable. Please try again shortly.');
  }
}

// ============================================================================
// 8. ALGORITHM COMPARISON: BFS VS DIJKSTRA (PANEL 7)
// ============================================================================

async function runAlgorithmComparison() {
  const origin = document.getElementById('compare-origin-select')?.value;
  const dest = document.getElementById('compare-dest-select')?.value;

  if (!origin || !dest) {
    showNotice('Select both an origin and destination to compare routing algorithms.');
    return;
  }

  const compareButton = document.getElementById('btn-run-comparison');
  const compareLabel = compareButton?.querySelector('span');
  const previousLabel = compareLabel?.textContent;
  if (compareButton) compareButton.disabled = true;
  if (compareLabel) compareLabel.textContent = 'Running both algorithms…';

  try {
    const [dijkstraRes, bfsRes] = await Promise.all([
      fetch(`${API_BASE}/route`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ origin, destination: dest, algorithm: 'dijkstra' })
      }),
      fetch(`${API_BASE}/route`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ origin, destination: dest, algorithm: 'bfs' })
      })
    ]);

    const dijkstraData = await dijkstraRes.json();
    const bfsData = await bfsRes.json();
    if (!dijkstraRes.ok || !bfsRes.ok) {
      showNotice(friendlyError(!dijkstraRes.ok ? dijkstraData : bfsData, 'Algorithm comparison failed.'));
      return;
    }

    state.dijkstraRoute = dijkstraData;
    state.bfsRoute = bfsData;

    const resultsContainer = document.getElementById('compare-results-container');
    if (resultsContainer) resultsContainer.classList.remove('hidden');

    // Populate Dijkstra Card
    document.getElementById('cmp-dijkstra-dist').textContent = dijkstraData.found ? `${dijkstraData.distance_km.toFixed(2)} km` : 'Unreachable';
    document.getElementById('cmp-dijkstra-hops').textContent = dijkstraData.found ? dijkstraData.hop_count : 0;
    renderWaypointsPills(document.getElementById('cmp-dijkstra-path'), dijkstraData.path_nodes || [], origin, null, dest);
    document.getElementById('cmp-dijkstra-summary').textContent = dijkstraData.found
      ? `Shortest weighted route: ${dijkstraData.distance_km.toFixed(2)} km across ${dijkstraData.hop_count} road segments.`
      : 'No route is available because the connecting roads are blocked.';

    // Populate BFS Card
    document.getElementById('cmp-bfs-dist').textContent = bfsData.found ? `${bfsData.distance_km.toFixed(2)} km` : 'Unreachable';
    document.getElementById('cmp-bfs-hops').textContent = bfsData.found ? bfsData.hop_count : 0;
    renderWaypointsPills(document.getElementById('cmp-bfs-path'), bfsData.path_nodes || [], origin, null, dest);
    document.getElementById('cmp-bfs-summary').textContent = bfsData.found
      ? `Fewest road segments: ${bfsData.hop_count} hops; route distance ${bfsData.distance_km.toFixed(2)} km.`
      : 'No route is available because the connecting roads are blocked.';

    // Populate Comparison Insight
    const insightText = document.getElementById('cmp-insight-text');
    if (!dijkstraData.found && !bfsData.found) {
      insightText.textContent = `Both algorithms confirm that destination [${dest}] is completely unreachable from origin [${origin}].`;
    } else {
      const dDist = dijkstraData.distance_km.toFixed(2);
      const bDist = bfsData.distance_km.toFixed(2);
      const dHops = dijkstraData.hop_count;
      const bHops = bfsData.hop_count;

      if (dDist === bDist && dHops === bHops) {
        insightText.textContent = `Both algorithms identified identical optimal trajectories (${dDist} km across ${dHops} road segments). In this topology, minimum distance aligns with minimum road segments.`;
      } else if (parseFloat(dDist) < parseFloat(bDist)) {
        insightText.textContent = `Dijkstra's Algorithm achieved a shorter travel distance (${dDist} km vs ${bDist} km), taking ${dHops} segments. BFS prioritized the fewest road hops (${bHops} segments), which resulted in a longer total route (+${(bDist - dDist).toFixed(2)} km).`;
      } else {
        insightText.textContent = `Dijkstra optimized total metric distance (${dDist} km, ${dHops} hops) using a Min-Heap Priority Queue. BFS traversed the graph by shortest hop count (${bHops} hops, ${bDist} km) using a FIFO Queue.`;
      }
    }

    // Default to showing Dijkstra on map
    showDijkstraOnMap();
  } catch (err) {
    console.error('Error comparing algorithms:', err);
    showNotice('Routing engine is temporarily unavailable. Please try again shortly.');
  } finally {
    if (compareButton) compareButton.disabled = false;
    if (compareLabel && previousLabel) compareLabel.textContent = previousLabel;
  }
}

function showDijkstraOnMap() {
  if (!state.dijkstraRoute || !state.dijkstraRoute.found) return;
  clearRouteLayers();
  drawPolylineRoute(
    state.dijkstraRoute.path_nodes,
    '#f59e0b',
    5,
    null,
    `⚡ Dijkstra Min-Distance: ${state.dijkstraRoute.origin} ➔ ${state.dijkstraRoute.destination} (${state.dijkstraRoute.distance_km.toFixed(2)} km, ${state.dijkstraRoute.hop_count} hops)`
  );
}

function showBfsOnMap() {
  if (!state.bfsRoute || !state.bfsRoute.found) return;
  clearRouteLayers();
  drawPolylineRoute(
    state.bfsRoute.path_nodes,
    '#8b5cf6',
    5,
    '6, 6',
    `⚡ BFS Min-Hops: ${state.bfsRoute.origin} ➔ ${state.bfsRoute.destination} (${state.bfsRoute.distance_km.toFixed(2)} km, ${state.bfsRoute.hop_count} hops)`
  );
}

// ============================================================================
// 9. SIMULATION RESET
// ============================================================================

async function resetSimulation() {
  const resetButton = document.getElementById('header-reset-btn');
  if (resetButton) resetButton.disabled = true;
  try {
    const res = await fetch(`${API_BASE}/reset`, { method: 'POST' });
    const data = await readJsonResponse(res);
    if (res.ok && data.status === 'success') {
      state.selectedIncidentId = null;
      state.blockedRoads.clear();
      state.allocatedAmbulances.clear();
      state.activeMission = null;
      state.dijkstraRoute = null;
      state.bfsRoute = null;

      clearRouteLayers();

      // Reset UI elements
      const incidentSelect = document.getElementById('incident-select');
      if (incidentSelect) incidentSelect.value = '';

      const dispatchBtn = document.getElementById('dispatch-btn');
      if (dispatchBtn) dispatchBtn.disabled = true;

      // Hide results
      ['amb-details', 'hosp-details', 'route-details', 'detour-result-box', 'compare-results-container'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.classList.add('hidden');
      });

      ['amb-empty-state', 'hosp-empty-state', 'route-empty-state'].forEach(id => {
        const el = document.getElementById(id);
        if (el) el.classList.remove('hidden');
      });

      const ambChip = document.getElementById('amb-status-chip');
      if (ambChip) {
        ambChip.textContent = 'Ready';
        ambChip.className = 'chip chip-blue';
      }

      await fetchCityData();
      recenterMap();
      showNotice('Simulation reset. Roads are open and ambulances are available.', 'success');
    } else {
      showNotice(friendlyError(data, 'The simulation could not be reset.'));
    }
  } catch (err) {
    console.error('Error resetting simulation:', err);
    showNotice('The simulation could not be reset. Check the backend connection.');
  } finally {
    if (resetButton) resetButton.disabled = false;
  }
}

// ============================================================================
// 10. EVENT LISTENERS
// ============================================================================

function setupEventListeners() {
  // Incident Dropdown
  const incidentSelect = document.getElementById('incident-select');
  if (incidentSelect) {
    incidentSelect.addEventListener('change', (e) => {
      selectEmergencyLocation(e.target.value);
    });
  }

  // Dispatch Button
  const dispatchBtn = document.getElementById('dispatch-btn');
  if (dispatchBtn) {
    dispatchBtn.addEventListener('click', executeEmergencyDispatch);
  }

  // Road Blockage Buttons
  const blockRoadBtn = document.getElementById('btn-block-road');
  if (blockRoadBtn) {
    blockRoadBtn.addEventListener('click', () => {
      const roadId = document.getElementById('road-select')?.value;
      if (roadId) blockRoad(roadId);
      else showNotice('Select a road segment to block.');
    });
  }

  const reopenRoadBtn = document.getElementById('btn-reopen-road');
  if (reopenRoadBtn) {
    reopenRoadBtn.addEventListener('click', () => {
      const roadId = document.getElementById('road-select')?.value;
      if (roadId) reopenRoad(roadId);
      else showNotice('Select a road segment to reopen.');
    });
  }

  // Detour Button
  const detourBtn = document.getElementById('btn-recalculate-detour');
  if (detourBtn) {
    detourBtn.addEventListener('click', recalculateDetour);
  }

  // Algorithm Comparison Buttons
  const runCompareBtn = document.getElementById('btn-run-comparison');
  if (runCompareBtn) {
    runCompareBtn.addEventListener('click', runAlgorithmComparison);
  }

  const showDijkstraBtn = document.getElementById('btn-show-dijkstra-map');
  if (showDijkstraBtn) {
    showDijkstraBtn.addEventListener('click', showDijkstraOnMap);
  }

  const showBfsBtn = document.getElementById('btn-show-bfs-map');
  if (showBfsBtn) {
    showBfsBtn.addEventListener('click', showBfsOnMap);
  }

  // Header Reset Button
  const resetBtn = document.getElementById('header-reset-btn');
  if (resetBtn) {
    resetBtn.addEventListener('click', resetSimulation);
  }

  // Floating Map Controls
  const mapResetBtn = document.getElementById('btn-map-reset-view');
  if (mapResetBtn) {
    mapResetBtn.addEventListener('click', recenterMap);
  }

  const mapClearBtn = document.getElementById('btn-map-clear-routes');
  if (mapClearBtn) {
    mapClearBtn.addEventListener('click', clearRouteLayers);
  }
}
