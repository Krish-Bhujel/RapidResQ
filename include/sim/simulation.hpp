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

    void reset();
    void triggerManualIncident(int nodeId, IncidentSeverity severity);
    void triggerChaosEvent();

    std::vector<Incident> getActiveIncidents() const;

    struct AmbulanceRenderState {
        bool isMoving = false;
        std::vector<int> currentEdgeNodes;
        float progress = 0.f;
        std::vector<int> fullRemainingRoute;
        int ticksRemainingTotal = 0;
        bool headingToIncident = false;
    };

    AmbulanceRenderState getAmbulanceRenderState(int ambulanceId, float subTickFraction) const;

private:
    enum class TravelPhase { ToIncident, OnScene, ReturningToHospital };

    struct ActiveTravel {
        int ambulanceId;
        int incidentId;
        int incidentNodeId;
        int destinationNodeId;
        TravelPhase phase;
        std::vector<int> remainingRoute;
        int ticksRemainingOnEdge = 0;
        int totalTicksOnEdge = 0;
    };

    void log(const std::string& message);
    Hospital* findHospital(int id);
    Ambulance* findAmbulance(int id);
    bool rerouteFrom(ActiveTravel& t, int fromNode, bool keepCurrentEdge);

    Graph& graph;
    IPathfinder& pathfinder;
    std::vector<Ambulance>& ambulances;
    std::vector<Hospital>& hospitals;
    Dispatcher dispatcher;

    std::vector<Incident> scenarioIncidents;
    std::vector<Incident> waitingIncidents;
    std::vector<ActiveTravel> activeTravels;
    std::vector<Event> eventLog;

    std::vector<BlockEvent> scenarioBlocks;
    int nextBlockIndex = 0;

    std::unordered_map<int, Incident> incidentsById;
    int nextManualIncidentId = 100000;

    int currentTick_ = 0;
    int nextScenarioIndex = 0;
    static const int ON_SCENE_DURATION = 2;
};