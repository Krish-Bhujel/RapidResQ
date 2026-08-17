#include "sim/simulation.hpp"
#include "sim/scenarioloader.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

Simulation::Simulation(Graph& g, IPathfinder& p, std::vector<Ambulance>& a, std::vector<Hospital>& h)
    : graph(g), pathfinder(p), ambulances(a), hospitals(h), dispatcher(g, p, a) {}

bool Simulation::loadScenario(const std::string& path) {
    return ScenarioLoader::load(path, scenarioIncidents, scenarioBlocks);
}

void Simulation::log(const std::string& message) {
    Event e;
    e.tick = currentTick_;
    e.message = message;
    eventLog.push_back(e);
}

Hospital* Simulation::findHospital(int id) {
    for (int i = 0; i < hospitals.size(); i++) {
        if (hospitals[i].id == id) return &hospitals[i];
    }
    return nullptr;
}

Ambulance* Simulation::findAmbulance(int id) {
    for (int i = 0; i < ambulances.size(); i++) {
        if (ambulances[i].id == id) return &ambulances[i];
    }
    return nullptr;
}

void Simulation::reset() {
    waitingIncidents.clear();
    activeTravels.clear();
    eventLog.clear();
    incidentsById.clear();
    currentTick_ = 0;
    nextScenarioIndex = 0;
    nextBlockIndex = 0;
}

void Simulation::triggerManualIncident(int nodeId, IncidentSeverity severity) {
    Incident inc;
    inc.id = nextManualIncidentId;
    nextManualIncidentId++;
    inc.triggerTick = currentTick_;
    inc.nodeId = nodeId;
    inc.severity = severity;
    inc.status = IncidentStatus::Pending;

    incidentsById[inc.id] = inc;
    waitingIncidents.push_back(inc);

    log("Incident triggered at node " + std::to_string(nodeId));
}

void Simulation::triggerChaosEvent() {
    log("=== CHAOS EVENT TRIGGERED ===");

    std::vector<std::pair<int,int>> unblocked;
    for (int id : graph.getAllNodeIds()) {
        std::vector<Edge> edges = graph.getNeighbours(id);
        for (int i = 0; i < edges.size(); i++) {
            if (id < edges[i].to) {
                unblocked.push_back(std::make_pair(id, edges[i].to));
            }
        }
    }

    if (unblocked.size() > 0) {
        int pick = std::rand() % unblocked.size();
        graph.blockEdge(unblocked[pick].first, unblocked[pick].second);
        log("CHAOS: road blocked between node " + std::to_string(unblocked[pick].first) +
            " and node " + std::to_string(unblocked[pick].second));
    }

    std::vector<int> ids = graph.getAllNodeIds();
    if (ids.size() > 0) {
        int count = 1 + (std::rand() % 3);
        for (int i = 0; i < count; i++) {
            int node = ids[std::rand() % ids.size()];
            IncidentSeverity sev = static_cast<IncidentSeverity>(std::rand() % 3);
            triggerManualIncident(node, sev);
        }
    }
}

std::vector<Incident> Simulation::getActiveIncidents() const {
    std::vector<Incident> result;
    for (auto pair : incidentsById) {
        if (pair.second.status != IncidentStatus::Resolved) {
            result.push_back(pair.second);
        }
    }
    return result;
}

bool Simulation::rerouteFrom(ActiveTravel& t, int fromNode, bool keepCurrentEdge) {
    Path alt = pathfinder.findPath(graph, fromNode, t.destinationNodeId);
    if (alt.totalCost < 0 || alt.nodes.size() == 0) return false;

    std::vector<int> newRoute;
    if (keepCurrentEdge && t.remainingRoute.size() >= 2) {
        newRoute.push_back(t.remainingRoute[0]);
        newRoute.push_back(t.remainingRoute[1]);
        for (int k = 1; k < alt.nodes.size(); k++) newRoute.push_back(alt.nodes[k]);
    } else {
        newRoute = alt.nodes;
    }
    t.remainingRoute = newRoute;
    return true;
}

void Simulation::tick() {
    // 1. Apply scheduled road blocks from the scenario file
    while (nextBlockIndex < scenarioBlocks.size() &&
           scenarioBlocks[nextBlockIndex].tick == currentTick_) {
        BlockEvent b = scenarioBlocks[nextBlockIndex];
        nextBlockIndex++;
        graph.blockEdge(b.fromNode, b.toNode);
        log("Road blocked between node " + std::to_string(b.fromNode) +
            " and node " + std::to_string(b.toNode) + ".");
    }

    // 2. Reroute checks
    for (int idx = 0; idx < activeTravels.size(); idx++) {
        ActiveTravel& t = activeTravels[idx];
        if (t.phase == TravelPhase::OnScene) continue;
        if (t.remainingRoute.size() < 2) continue;

        int curFrom = t.remainingRoute[0];
        int curTo = t.remainingRoute[1];

        // Case A: currently-driven edge just got blocked -> turn back now
        if (graph.isEdgeBlocked(curFrom, curTo)) {
            int ticksUsed = t.totalTicksOnEdge - t.ticksRemainingOnEdge;
            if (ticksUsed < 1) ticksUsed = 1;

            Path cont = pathfinder.findPath(graph, curFrom, t.destinationNodeId);
            std::vector<int> newRoute;
            newRoute.push_back(curTo);
            newRoute.push_back(curFrom);
            if (cont.totalCost >= 0) {
                for (int k = 1; k < cont.nodes.size(); k++) newRoute.push_back(cont.nodes[k]);
            }

            t.remainingRoute = newRoute;
            t.ticksRemainingOnEdge = ticksUsed;

            log("Ambulance " + std::to_string(t.ambulanceId) + " hit a road block mid-route and is turning back.");
            continue;
        }

        // Case B: a future edge (not yet entered) is blocked
        if (t.remainingRoute.size() >= 3) {
            bool blockedAhead = false;
            for (int i = 1; i + 1 < t.remainingRoute.size(); i++) {
                if (graph.isEdgeBlocked(t.remainingRoute[i], t.remainingRoute[i + 1])) {
                    blockedAhead = true;
                    break;
                }
            }
            if (blockedAhead) {
                int fromNode = t.remainingRoute[1];
                bool ok = rerouteFrom(t, fromNode, true);
                if (ok) {
                    log("Ambulance " + std::to_string(t.ambulanceId) + " detected a blocked road ahead and is rerouting.");
                } else {
                    log("Ambulance " + std::to_string(t.ambulanceId) + " found no alternate route yet; will keep retrying.");
                }
            }
        }
    }

    // 3. Trigger scenario incidents scheduled for this tick
    while (nextScenarioIndex < scenarioIncidents.size() &&
           scenarioIncidents[nextScenarioIndex].triggerTick == currentTick_) {
        Incident inc = scenarioIncidents[nextScenarioIndex];
        nextScenarioIndex++;
        incidentsById[inc.id] = inc;
        log("Incident detected at node " + std::to_string(inc.nodeId));
        waitingIncidents.push_back(inc);
    }

    // 4. Dispatch waiting incidents
    for (int idx = 0; idx < waitingIncidents.size(); idx++) {
        Incident& incident = waitingIncidents[idx];

        DispatchResult result = dispatcher.assign(incident);

        if (result.success) {
            incidentsById[incident.id] = incident;
            log("Ambulance " + std::to_string(result.ambulanceId) + " assigned. Route cost: " +
                std::to_string(result.route.totalCost));

            ActiveTravel t;
            t.ambulanceId = result.ambulanceId;
            t.incidentId = incident.id;
            t.incidentNodeId = incident.nodeId;
            t.destinationNodeId = incident.nodeId;
            t.phase = TravelPhase::ToIncident;
            t.remainingRoute = result.route.nodes;
            if (t.remainingRoute.size() < 2) {
                t.remainingRoute.clear();
                t.remainingRoute.push_back(incident.nodeId);
                t.remainingRoute.push_back(incident.nodeId);
            }
            double w = graph.getEdgeWeight(t.remainingRoute[0], t.remainingRoute[1]);
            if (w < 0) w = 1.0;
            t.totalTicksOnEdge = std::max(1, static_cast<int>(std::round(w)));
            t.ticksRemainingOnEdge = t.totalTicksOnEdge;

            activeTravels.push_back(t);
            waitingIncidents.erase(waitingIncidents.begin() + idx);
            idx--; // stay at the same index since we removed one
            continue;
        }

        // No idle ambulance -> try redirecting the closest returning one
        int bestIdx = -1;
        double bestCost = std::numeric_limits<double>::max();
        Path bestPath;

        for (int j = 0; j < activeTravels.size(); j++) {
            if (activeTravels[j].phase != TravelPhase::ReturningToHospital) continue;
            if (activeTravels[j].remainingRoute.size() == 0) continue;

            int fromNode = activeTravels[j].remainingRoute[0];
            Path p = pathfinder.findPath(graph, fromNode, incident.nodeId);
            if (p.totalCost >= 0 && p.totalCost < bestCost) {
                bestCost = p.totalCost;
                bestIdx = j;
                bestPath = p;
            }
        }

        if (bestIdx != -1) {
            ActiveTravel& t = activeTravels[bestIdx];
            Ambulance* amb = findAmbulance(t.ambulanceId);

            std::vector<int> newRoute;
            if (t.remainingRoute.size() >= 2) {
                newRoute.push_back(t.remainingRoute[0]);
                newRoute.push_back(t.remainingRoute[1]);
                for (int k = 1; k < bestPath.nodes.size(); k++) newRoute.push_back(bestPath.nodes[k]);
            } else {
                newRoute = bestPath.nodes;
            }

            t.remainingRoute = newRoute;
            t.phase = TravelPhase::ToIncident;
            t.incidentId = incident.id;
            t.incidentNodeId = incident.nodeId;
            t.destinationNodeId = incident.nodeId;

            if (amb) amb->status = AmbulanceStatus::EnRouteToIncident;

            Incident updated = incident;
            updated.status = IncidentStatus::Assigned;
            updated.assignedAmbulanceId = t.ambulanceId;
            incidentsById[incident.id] = updated;

            log("Ambulance " + std::to_string(t.ambulanceId) + " redirected mid-return to handle new incident at node " +
                std::to_string(incident.nodeId) + ".");

            waitingIncidents.erase(waitingIncidents.begin() + idx);
            idx--;
            continue;
        }

        log("No ambulance available yet for incident at node " + std::to_string(incident.nodeId));
    }

    // 5. Progress every active travel
    for (int idx = 0; idx < activeTravels.size(); idx++) {
        ActiveTravel& t = activeTravels[idx];

        if (t.phase == TravelPhase::OnScene) {
            t.ticksRemainingOnEdge--;
            if (t.ticksRemainingOnEdge <= 0) {
                Ambulance* amb = findAmbulance(t.ambulanceId);
                log("Incident " + std::to_string(t.incidentId) + " resolved. Ambulance " +
                    std::to_string(t.ambulanceId) + " returning to hospital.");

                if (incidentsById.count(t.incidentId) > 0) {
                    incidentsById[t.incidentId].status = IncidentStatus::Resolved;
                }

                if (amb) {
                    amb->status = AmbulanceStatus::ReturningToHospital;
                    Hospital* home = findHospital(amb->homeHospitalId);
                    int homeNode = home ? home->nodeId : amb->currentNodeId;
                    t.destinationNodeId = homeNode;

                    if (!rerouteFrom(t, t.incidentNodeId, false)) {
                        t.remainingRoute.clear();
                        t.remainingRoute.push_back(t.incidentNodeId);
                    }
                    if (t.remainingRoute.size() < 2) {
                        t.remainingRoute.push_back(t.incidentNodeId);
                    }
                    double w = graph.getEdgeWeight(t.remainingRoute[0], t.remainingRoute[1]);
                    if (w < 0) w = 1.0;
                    t.totalTicksOnEdge = std::max(1, static_cast<int>(std::round(w)));
                    t.ticksRemainingOnEdge = t.totalTicksOnEdge;
                }
                t.phase = TravelPhase::ReturningToHospital;
            }
            continue;
        }

        if (t.remainingRoute.size() < 2) {
            t.ticksRemainingOnEdge = 0;
        } else {
            t.ticksRemainingOnEdge--;
        }

        if (t.ticksRemainingOnEdge > 0) continue;

        if (t.remainingRoute.size() >= 2) {
            t.remainingRoute.erase(t.remainingRoute.begin());
        }

        if (t.remainingRoute.size() <= 1) {
            if (t.phase == TravelPhase::ToIncident) {
                Ambulance* amb = findAmbulance(t.ambulanceId);
                log("Ambulance " + std::to_string(t.ambulanceId) + " arrived on scene.");

                if (amb) {
                    amb->status = AmbulanceStatus::OnScene;
                    amb->currentNodeId = t.incidentNodeId;
                }

                t.phase = TravelPhase::OnScene;
                t.remainingRoute.clear();
                t.remainingRoute.push_back(t.incidentNodeId);
                t.ticksRemainingOnEdge = ON_SCENE_DURATION;
                t.totalTicksOnEdge = ON_SCENE_DURATION;
            } else {
                Ambulance* amb = findAmbulance(t.ambulanceId);
                if (amb) {
                    amb->status = AmbulanceStatus::Idle;
                    amb->currentNodeId = t.destinationNodeId;
                }
                log("Ambulance " + std::to_string(t.ambulanceId) + " back at hospital and idle.");
                activeTravels.erase(activeTravels.begin() + idx);
                idx--;
            }
            continue;
        }

        // start next edge
        if (graph.isEdgeBlocked(t.remainingRoute[0], t.remainingRoute[1])) {
            if (!rerouteFrom(t, t.remainingRoute[0], false)) {
                t.ticksRemainingOnEdge = 1;
                t.totalTicksOnEdge = 1;
                continue;
            }
        }

        double w = graph.getEdgeWeight(t.remainingRoute[0], t.remainingRoute[1]);
        if (w < 0) w = 1.0;
        t.totalTicksOnEdge = std::max(1, static_cast<int>(std::round(w)));
        t.ticksRemainingOnEdge = t.totalTicksOnEdge;
    }

    currentTick_++;
}

bool Simulation::isFinished() const {
    return waitingIncidents.size() == 0 &&
           activeTravels.size() == 0 &&
           nextScenarioIndex >= scenarioIncidents.size() &&
           nextBlockIndex >= scenarioBlocks.size();
}

const std::vector<Event>& Simulation::getEventLog() const {
    return eventLog;
}

Simulation::AmbulanceRenderState Simulation::getAmbulanceRenderState(int ambulanceId, float subTickFraction) const {
    AmbulanceRenderState state;
    for (int i = 0; i < activeTravels.size(); i++) {
        const ActiveTravel& t = activeTravels[i];
        if (t.ambulanceId != ambulanceId) continue;
        if (t.phase == TravelPhase::OnScene) return state;

        state.isMoving = true;
        state.headingToIncident = (t.phase == TravelPhase::ToIncident);
        state.fullRemainingRoute = t.remainingRoute;

        if (t.remainingRoute.size() >= 2) {
            state.currentEdgeNodes.push_back(t.remainingRoute[0]);
            state.currentEdgeNodes.push_back(t.remainingRoute[1]);
        } else {
            state.currentEdgeNodes.push_back(t.remainingRoute[0]);
            state.currentEdgeNodes.push_back(t.remainingRoute[0]);
        }

        float doneTicks = static_cast<float>(t.totalTicksOnEdge - t.ticksRemainingOnEdge);
        float continuous = doneTicks + subTickFraction;
        if (t.totalTicksOnEdge > 0) {
            state.progress = std::min(1.f, continuous / static_cast<float>(t.totalTicksOnEdge));
        } else {
            state.progress = 1.f;
        }

        int totalTicks = t.ticksRemainingOnEdge;
        for (int i2 = 1; i2 + 1 < t.remainingRoute.size(); i2++) {
            double w = graph.getEdgeWeight(t.remainingRoute[i2], t.remainingRoute[i2 + 1]);
            if (w < 0) w = 1.0;
            totalTicks += std::max(1, static_cast<int>(std::round(w)));
        }
        state.ticksRemainingTotal = totalTicks;

        return state;
    }
    return state;
}