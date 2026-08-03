#include "sim/simulation.hpp"
#include "sim/scenarioloader.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>

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
    oss << "Manual incident triggered at node " << nodeId;
    log(oss.str());
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

void Simulation::tick() {
    // 1. Process any road blocks scheduled for this tick
    while (nextBlockIndex_ < scenarioBlocks_.size() &&
           scenarioBlocks_[nextBlockIndex_].tick == currentTick_) {
        const BlockEvent& b = scenarioBlocks_[nextBlockIndex_];
        nextBlockIndex_++;
        graph_.blockEdge(b.fromNode, b.toNode);
        std::ostringstream oss;
        oss << "Road blocked between node " << b.fromNode << " and node " << b.toNode
            << ". Active ambulances will reroute at next dispatch.";
        log(oss.str());
    }

    // 2. Trigger any scenario incidents due this tick
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

    // 3. Try to dispatch every waiting incident
    for (auto it = waitingIncidents_.begin(); it != waitingIncidents_.end(); ) {
        DispatchResult result = dispatcher_.assign(*it);
        if (result.success) {
            incidentsById_[it->id] = *it; // now Assigned, with assignedAmbulanceId set

            std::ostringstream oss;
            oss << "Ambulance " << result.ambulanceId << " assigned. Route cost: "
                << result.route.totalCost;
            log(oss.str());

            int ticks = std::max(1, static_cast<int>(std::round(result.route.totalCost)));
            activeTravels_.push_back({
                result.ambulanceId, it->id, it->nodeId,
                ticks, ticks, TravelPhase::ToIncident, result.route
            });

            it = waitingIncidents_.erase(it);
        } else {
            std::ostringstream oss;
            oss << "No ambulance available yet for incident at node " << it->nodeId;
            log(oss.str());
            ++it;
        }
    }

    // 4. Progress every active ambulance travel
    for (auto it = activeTravels_.begin(); it != activeTravels_.end(); ) {
        it->ticksRemaining--;

        if (it->ticksRemaining > 0) {
            ++it;
            continue;
        }

        Ambulance* amb = findAmbulance(it->ambulanceId);

        if (it->phase == TravelPhase::ToIncident) {
            std::ostringstream oss;
            oss << "Ambulance " << it->ambulanceId << " arrived on scene.";
            log(oss.str());

            if (amb) {
                amb->status = AmbulanceStatus::OnScene;
                amb->currentNodeId = it->incidentNodeId;
            }

            it->phase = TravelPhase::OnScene;
            it->ticksRemaining = ON_SCENE_DURATION;
            it->totalTicksForPhase = ON_SCENE_DURATION;
            it->currentRoute.nodes = {it->incidentNodeId};
            it->currentRoute.totalCost = 0;
            ++it;

        } else if (it->phase == TravelPhase::OnScene) {
            std::ostringstream oss;
            oss << "Incident " << it->incidentId << " resolved. Ambulance "
                << it->ambulanceId << " returning to hospital.";
            log(oss.str());

            auto incIt = incidentsById_.find(it->incidentId);
            if (incIt != incidentsById_.end()) {
                incIt->second.status = IncidentStatus::Resolved;
            }

            if (amb) {
                amb->status = AmbulanceStatus::ReturningToHospital;
                Hospital* home = findHospital(amb->homeHospitalId);
                int returnTicks = 1;
                Path back;
                if (home) {
                    back = pathfinder_.findPath(graph_, amb->currentNodeId, home->nodeId);
                    if (back.totalCost >= 0) {
                        returnTicks = std::max(1, static_cast<int>(std::round(back.totalCost)));
                    }
                }
                it->currentRoute = back;
                it->ticksRemaining = returnTicks;
                it->totalTicksForPhase = returnTicks;
            } else {
                it->ticksRemaining = 1;
                it->totalTicksForPhase = 1;
            }
            it->phase = TravelPhase::ReturningToHospital;
            ++it;

        } else { // ReturningToHospital finished
            if (amb) {
                amb->status = AmbulanceStatus::Idle;
                Hospital* home = findHospital(amb->homeHospitalId);
                if (home) amb->currentNodeId = home->nodeId;
                std::ostringstream oss;
                oss << "Ambulance " << it->ambulanceId << " back at hospital and idle.";
                log(oss.str());
            }
            it = activeTravels_.erase(it);
        }
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

        state.isMoving = true;
        state.routeNodes = t.currentRoute.nodes;
        state.ticksRemaining = t.ticksRemaining;
        state.headingToIncident = (t.phase == TravelPhase::ToIncident);

        float doneTicks = static_cast<float>(t.totalTicksForPhase - t.ticksRemaining);
        float continuous = doneTicks + subTickFraction;
        state.progress = (t.totalTicksForPhase > 0)
            ? std::min(1.f, continuous / static_cast<float>(t.totalTicksForPhase))
            : 1.f;
        return state;
    }
    return state;
}