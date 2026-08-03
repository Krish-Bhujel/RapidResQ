#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "graph/graph.hpp"
#include "algo/ipathfinder.hpp"
#include "sim/ambulance.hpp"
#include "sim/hospital.hpp"
#include "sim/incident.hpp"
#include "sim/dispatcher.hpp"
#include "sim/event.hpp"
#include "sim/block_event.hpp"

class Simulation {
public:
    Simulation(Graph& graph, IPathfinder& pathfinder,
               std::vector<Ambulance>& ambulances, std::vector<Hospital>& hospitals);

    bool loadScenario(const std::string& path);

    void tick();

    bool isFinished() const;

    const std::vector<Event>& getEventLog() const;

    int currentTick() const { return currentTick_; }

    // Clears all simulation progress (ticks, events, active travels, incidents)
    // back to a fresh start. Does NOT touch Graph/Ambulances/Hospitals —
    // caller is responsible for reloading those separately if a full reset
    // of blocked roads / ambulance positions is also wanted.
    void reset();

    // Immediately queues a new incident, independent of the loaded scenario.
    void triggerManualIncident(int nodeId, IncidentSeverity severity);

    // All incidents that have been triggered but not yet resolved.
    std::vector<Incident> getActiveIncidents() const;

    struct AmbulanceRenderState {
        bool isMoving = false;
        std::vector<int> routeNodes;
        float progress = 0.f;
        int ticksRemaining = 0;
        bool headingToIncident = false;
    };

    AmbulanceRenderState getAmbulanceRenderState(int ambulanceId, float subTickFraction) const;

private:
    enum class TravelPhase { ToIncident, OnScene, ReturningToHospital };

    struct ActiveTravel {
        int ambulanceId;
        int incidentId;
        int incidentNodeId;
        int ticksRemaining;
        int totalTicksForPhase;
        TravelPhase phase;
        Path currentRoute;
    };

    void log(const std::string& message);
    Hospital* findHospital(int id);
    Ambulance* findAmbulance(int id);

    Graph& graph_;
    IPathfinder& pathfinder_;
    std::vector<Ambulance>& ambulances_;
    std::vector<Hospital>& hospitals_;
    Dispatcher dispatcher_;

    std::vector<Incident> scenarioIncidents_;
    std::vector<Incident> waitingIncidents_;
    std::vector<ActiveTravel> activeTravels_;
    std::vector<Event> eventLog_;

    std::vector<BlockEvent> scenarioBlocks_;
    size_t nextBlockIndex_ = 0;

    std::unordered_map<int, Incident> incidentsById_;
    int nextManualIncidentId_ = 100000; // kept well above scenario ids to avoid collisions

    int currentTick_ = 0;
    size_t nextScenarioIndex_ = 0;
    static const int ON_SCENE_DURATION = 2;
};