#include "sim/simulation.hpp"
#include "sim/scenarioloader.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

Simulation::Simulation(Graph& graph, IPathfinder& pathfinder,
                        std::vector<Ambulance>& ambulances, std::vector<Hospital>& hospitals)
    : graph_(graph), pathfinder_(pathfinder), ambulances_(ambulances), hospitals_(hospitals),
      dispatcher_(graph, pathfinder, ambulances) {}

bool Simulation::loadScenario(const std::string& path) {
    return ScenarioLoader::load(path, scenarioIncidents_, scenarioBlocks_);
}

void Simulation::log(const std::string& message) {
    eventLog_.push_back({currentTick_, message});
}

Hospital* Simulation::findHospital(int id) {
    for (auto& h : hospitals_) if (h.id == id) return &h;
    return nullptr;
}

Ambulance* Simulation::findAmbulance(int id) {
    for (auto& a : ambulances_) if (a.id == id) return &a;
    return nullptr;
}

void Simulation::reset() {
    waitingIncidents_.clear();
    activeTravels_.clear();
    eventLog_.clear();
    incidentsById_.clear();
    currentTick_ = 0;
    nextScenarioIndex_ = 0;
    nextBlockIndex_ = 0;
}

void Simulation::triggerManualIncident(int nodeId, IncidentSeverity severity) {
    Incident inc;
    inc.id = nextManualIncidentId_++;
    inc.triggerTick = currentTick_;
    inc.nodeId = nodeId;
    inc.severity = severity;
    inc.status = IncidentStatus::Pending;

    incidentsById_[inc.id] = inc;
    waitingIncidents_.push_back(inc);

    std::ostringstream oss;
    oss << "Incident triggered at node " << nodeId;
    log(oss.str());
}

void Simulation::triggerChaosEvent() {
    log("=== CHAOS EVENT TRIGGERED ===");

    auto unblocked = graph_.getUnblockedEdges();
    if (!unblocked.empty()) {
        auto& e = unblocked[std::rand() % unblocked.size()];
        graph_.blockEdge(e.first, e.second);
        std::ostringstream oss;
        oss << "CHAOS: road blocked between node " << e.first << " and node " << e.second;
        log(oss.str());
    }

    std::vector<int> ids = graph_.getAllNodeIds();
    if (!ids.empty()) {
        int count = 1 + (std::rand() % 3); // 1-3 incidents
        for (int i = 0; i < count; ++i) {
            int node = ids[std::rand() % ids.size()];
            IncidentSeverity sev = static_cast<IncidentSeverity>(std::rand() % 3);
            triggerManualIncident(node, sev);
        }
    }
}

std::vector<Incident> Simulation::getActiveIncidents() const {
    std::vector<Incident> result;
    for (const auto& [id, inc] : incidentsById_) {
        if (inc.status != IncidentStatus::Resolved) {
            result.push_back(inc);
        }
    }
    return result;
}

bool Simulation::rerouteFrom(ActiveTravel& t, int fromNode, bool keepCurrentEdge) {
    Path alt = pathfinder_.findPath(graph_, fromNode, t.destinationNodeId);
    if (alt.totalCost < 0 || alt.nodes.empty()) return false;

    std::vector<int> newRoute;
    if (keepCurrentEdge && t.remainingRoute.size() >= 2) {
        newRoute.push_back(t.remainingRoute[0]);
        newRoute.push_back(t.remainingRoute[1]);
        for (size_t k = 1; k < alt.nodes.size(); ++k) newRoute.push_back(alt.nodes[k]);
    } else {
        newRoute = alt.nodes;
    }
    t.remainingRoute = newRoute;
    return true;
}

void Simulation::tick() {
    // 1. Process any road blocks scheduled for this tick
    while (nextBlockIndex_ < scenarioBlocks_.size() &&
           scenarioBlocks_[nextBlockIndex_].tick == currentTick_) {
        const BlockEvent& b = scenarioBlocks_[nextBlockIndex_];
        nextBlockIndex_++;
        graph_.blockEdge(b.fromNode, b.toNode);
        std::ostringstream oss;
        oss << "Road blocked between node " << b.fromNode << " and node " << b.toNode << ".";
        log(oss.str());
    }

    // 2. Reroute check.
    for (auto& t : activeTravels_) {
        if (t.phase == TravelPhase::OnScene) continue;
        if (t.remainingRoute.size() < 2) continue;

        int curFrom = t.remainingRoute[0];
        int curTo = t.remainingRoute[1];

        // Case A: the edge it is CURRENTLY DRIVING ON just became blocked.
        // Turn back immediately, mirroring how far along it already was.
        if (graph_.isEdgeBlocked(curFrom, curTo)) {
            int ticksUsed = t.totalTicksOnEdge - t.ticksRemainingOnEdge;
            if (ticksUsed < 1) ticksUsed = 1; // avoid an instant, teleport-like snap-back

            Path cont = pathfinder_.findPath(graph_, curFrom, t.destinationNodeId);
            std::vector<int> newRoute = {curTo, curFrom}; // drive back the way it came
            if (cont.totalCost >= 0) {
                for (size_t k = 1; k < cont.nodes.size(); ++k) newRoute.push_back(cont.nodes[k]);
            }

            t.remainingRoute = newRoute;
            // Same physical edge, so it takes just as long to retrace it as it took to get here.
            t.ticksRemainingOnEdge = ticksUsed;
            // totalTicksOnEdge stays as-is (same edge length) so progress interpolation stays smooth.

            std::ostringstream oss;
            oss << "Ambulance " << t.ambulanceId << " hit a road block mid-route and is turning back.";
            log(oss.str());
            continue; // don't also run the "ahead" check this tick
        }

        // Case B: an edge FURTHER DOWN the route (not yet entered) is blocked.
        // Reroute once it reaches the node just before that edge.
        if (t.remainingRoute.size() >= 3) {
            bool blockedAhead = false;
            for (size_t i = 1; i + 1 < t.remainingRoute.size(); ++i) {
                if (graph_.isEdgeBlocked(t.remainingRoute[i], t.remainingRoute[i + 1])) {
                    blockedAhead = true;
                    break;
                }
            }

            if (blockedAhead) {
                int fromNode = t.remainingRoute[1];
                bool ok = rerouteFrom(t, fromNode, true);
                std::ostringstream oss;
                if (ok) {
                    oss << "Ambulance " << t.ambulanceId << " detected a blocked road ahead and is rerouting.";
                } else {
                    oss << "Ambulance " << t.ambulanceId << " found no alternate route yet; will keep retrying.";
                }
                log(oss.str());
            }
        }
    }

    // 3. Trigger any scenario incidents due this tick
    while (nextScenarioIndex_ < scenarioIncidents_.size() &&
           scenarioIncidents_[nextScenarioIndex_].triggerTick == currentTick_) {
        Incident inc = scenarioIncidents_[nextScenarioIndex_];
        nextScenarioIndex_++;
        incidentsById_[inc.id] = inc;
        std::ostringstream oss;
        oss << "Incident detected at node " << inc.nodeId;
        log(oss.str());
        waitingIncidents_.push_back(inc);
    }

    // 4. Try to dispatch every waiting incident: first idle ambulances, then
    //    fall back to redirecting the closest currently-returning ambulance.
    for (auto it = waitingIncidents_.begin(); it != waitingIncidents_.end(); ) {
        DispatchResult result = dispatcher_.assign(*it);

        if (result.success) {
            incidentsById_[it->id] = *it;

            std::ostringstream oss;
            oss << "Ambulance " << result.ambulanceId << " assigned. Route cost: " << result.route.totalCost;
            log(oss.str());

            ActiveTravel t;
            t.ambulanceId = result.ambulanceId;
            t.incidentId = it->id;
            t.incidentNodeId = it->nodeId;
            t.destinationNodeId = it->nodeId;
            t.phase = TravelPhase::ToIncident;
            t.remainingRoute = result.route.nodes;
            if (t.remainingRoute.size() < 2) {
                t.remainingRoute = {it->nodeId, it->nodeId};
            }
            double w = graph_.getEdgeWeight(t.remainingRoute[0], t.remainingRoute[1]);
            if (w < 0) w = 1.0;
            t.totalTicksOnEdge = std::max(1, static_cast<int>(std::round(w)));
            t.ticksRemainingOnEdge = t.totalTicksOnEdge;

            activeTravels_.push_back(t);
            it = waitingIncidents_.erase(it);
            continue;
        }

        // No idle ambulance available — try redirecting the closest returning one.
        ActiveTravel* best = nullptr;
        double bestCost = std::numeric_limits<double>::max();
        Path bestPath;

        for (auto& t : activeTravels_) {
            if (t.phase != TravelPhase::ReturningToHospital) continue;
            if (t.remainingRoute.empty()) continue;

            int fromNode = t.remainingRoute[0];
            Path p = pathfinder_.findPath(graph_, fromNode, it->nodeId);
            if (p.totalCost >= 0 && p.totalCost < bestCost) {
                bestCost = p.totalCost;
                best = &t;
                bestPath = p;
            }
        }

        if (best) {
            Ambulance* amb = findAmbulance(best->ambulanceId);

            std::vector<int> newRoute;
            if (best->remainingRoute.size() >= 2) {
                newRoute.push_back(best->remainingRoute[0]);
                newRoute.push_back(best->remainingRoute[1]);
                for (size_t k = 1; k < bestPath.nodes.size(); ++k) newRoute.push_back(bestPath.nodes[k]);
            } else {
                newRoute = bestPath.nodes.empty() ? std::vector<int>{it->nodeId, it->nodeId} : bestPath.nodes;
            }

            best->remainingRoute = newRoute;
            best->phase = TravelPhase::ToIncident;
            best->incidentId = it->id;
            best->incidentNodeId = it->nodeId;
            best->destinationNodeId = it->nodeId;

            if (amb) amb->status = AmbulanceStatus::EnRouteToIncident;

            Incident updated = *it;
            updated.status = IncidentStatus::Assigned;
            updated.assignedAmbulanceId = best->ambulanceId;
            incidentsById_[it->id] = updated;

            std::ostringstream oss;
            oss << "Ambulance " << best->ambulanceId << " redirected mid-return to handle new incident at node "
                << it->nodeId << ".";
            log(oss.str());

            it = waitingIncidents_.erase(it);
            continue;
        }

        std::ostringstream oss;
        oss << "No ambulance available yet for incident at node " << it->nodeId;
        log(oss.str());
        ++it;
    }

    // 5. Progress every active travel
    for (auto it = activeTravels_.begin(); it != activeTravels_.end(); ) {
        if (it->phase == TravelPhase::OnScene) {
            it->ticksRemainingOnEdge--;
            if (it->ticksRemainingOnEdge <= 0) {
                Ambulance* amb = findAmbulance(it->ambulanceId);
                std::ostringstream oss;
                oss << "Incident " << it->incidentId << " resolved. Ambulance "
                    << it->ambulanceId << " returning to hospital.";
                log(oss.str());

                auto incIt = incidentsById_.find(it->incidentId);
                if (incIt != incidentsById_.end()) incIt->second.status = IncidentStatus::Resolved;

                if (amb) {
                    amb->status = AmbulanceStatus::ReturningToHospital;
                    Hospital* home = findHospital(amb->homeHospitalId);
                    int homeNode = home ? home->nodeId : amb->currentNodeId;
                    it->destinationNodeId = homeNode;

                    if (!rerouteFrom(*it, it->incidentNodeId, false)) {
                        it->remainingRoute = {it->incidentNodeId};
                    }
                    if (it->remainingRoute.size() < 2) {
                        it->remainingRoute = {it->incidentNodeId, it->incidentNodeId};
                    }
                    double w = graph_.getEdgeWeight(it->remainingRoute[0], it->remainingRoute[1]);
                    if (w < 0) w = 1.0;
                    it->totalTicksOnEdge = std::max(1, static_cast<int>(std::round(w)));
                    it->ticksRemainingOnEdge = it->totalTicksOnEdge;
                }
                it->phase = TravelPhase::ReturningToHospital;
            }
            ++it;
            continue;
        }

        // Moving phase (ToIncident or ReturningToHospital)
        if (it->remainingRoute.size() < 2) {
            it->ticksRemainingOnEdge = 0;
        } else {
            it->ticksRemainingOnEdge--;
        }

        if (it->ticksRemainingOnEdge > 0) {
            ++it;
            continue;
        }

        if (it->remainingRoute.size() >= 2) {
            it->remainingRoute.erase(it->remainingRoute.begin());
        }

        if (it->remainingRoute.size() <= 1) {
            // Arrived at final destination of this phase
            if (it->phase == TravelPhase::ToIncident) {
                Ambulance* amb = findAmbulance(it->ambulanceId);
                std::ostringstream oss;
                oss << "Ambulance " << it->ambulanceId << " arrived on scene.";
                log(oss.str());

                if (amb) {
                    amb->status = AmbulanceStatus::OnScene;
                    amb->currentNodeId = it->incidentNodeId;
                }

                it->phase = TravelPhase::OnScene;
                it->remainingRoute = {it->incidentNodeId};
                it->ticksRemainingOnEdge = ON_SCENE_DURATION;
                it->totalTicksOnEdge = ON_SCENE_DURATION;
                ++it;
            } else {
                Ambulance* amb = findAmbulance(it->ambulanceId);
                if (amb) {
                    amb->status = AmbulanceStatus::Idle;
                    amb->currentNodeId = it->destinationNodeId;
                }
                std::ostringstream oss;
                oss << "Ambulance " << it->ambulanceId << " back at hospital and idle.";
                log(oss.str());
                it = activeTravels_.erase(it);
            }
            continue;
        }

        // Start the next edge
        if (graph_.isEdgeBlocked(it->remainingRoute[0], it->remainingRoute[1])) {
            if (!rerouteFrom(*it, it->remainingRoute[0], false)) {
                // stuck — retry next tick
                it->ticksRemainingOnEdge = 1;
                it->totalTicksOnEdge = 1;
                ++it;
                continue;
            }
        }

        double w = graph_.getEdgeWeight(it->remainingRoute[0], it->remainingRoute[1]);
        if (w < 0) w = 1.0;
        it->totalTicksOnEdge = std::max(1, static_cast<int>(std::round(w)));
        it->ticksRemainingOnEdge = it->totalTicksOnEdge;
        ++it;
    }

    currentTick_++;
}

bool Simulation::isFinished() const {
    return waitingIncidents_.empty() &&
           activeTravels_.empty() &&
           nextScenarioIndex_ >= scenarioIncidents_.size() &&
           nextBlockIndex_ >= scenarioBlocks_.size();
}

const std::vector<Event>& Simulation::getEventLog() const {
    return eventLog_;
}

Simulation::AmbulanceRenderState Simulation::getAmbulanceRenderState(int ambulanceId, float subTickFraction) const {
    AmbulanceRenderState state;
    for (const auto& t : activeTravels_) {
        if (t.ambulanceId != ambulanceId) continue;
        if (t.phase == TravelPhase::OnScene) return state; // isMoving stays false

        state.isMoving = true;
        state.headingToIncident = (t.phase == TravelPhase::ToIncident);
        state.fullRemainingRoute = t.remainingRoute;

        if (t.remainingRoute.size() >= 2) {
            state.currentEdgeNodes = {t.remainingRoute[0], t.remainingRoute[1]};
        } else {
            state.currentEdgeNodes = {t.remainingRoute[0], t.remainingRoute[0]};
        }

        float doneTicks = static_cast<float>(t.totalTicksOnEdge - t.ticksRemainingOnEdge);
        float continuous = doneTicks + subTickFraction;
        state.progress = (t.totalTicksOnEdge > 0)
            ? std::min(1.f, continuous / static_cast<float>(t.totalTicksOnEdge))
            : 1.f;

        int totalTicks = t.ticksRemainingOnEdge;
        for (size_t i = 1; i + 1 < t.remainingRoute.size(); ++i) {
            double w = graph_.getEdgeWeight(t.remainingRoute[i], t.remainingRoute[i + 1]);
            if (w < 0) w = 1.0;
            totalTicks += std::max(1, static_cast<int>(std::round(w)));
        }
        state.ticksRemainingTotal = totalTicks;

        return state;
    }
    return state;
}