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
    void triggerChaosEvent(); // blocks a random unblocked road + spawns 1-3 random incidents

    std::vector<Incident> getActiveIncidents() const;

    struct AmbulanceRenderState {
        bool isMoving = false;
        std::vector<int> currentEdgeNodes;   // exactly 2 nodes: the edge currently being driven
        float progress = 0.f;                // 0..1 progress along currentEdgeNodes
        std::vector<int> fullRemainingRoute; // current node -> destination, for drawing the path overlay
        int ticksRemainingTotal = 0;         // ETA to final destination of this phase
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
        std::vector<int> remainingRoute; // [0] = current/last-reached node ... [back] = destinationNodeId
        int ticksRemainingOnEdge = 0;
        int totalTicksOnEdge = 0;
    };

    void log(const std::string& message);
    Hospital* findHospital(int id);
    Ambulance* findAmbulance(int id);

    // Recomputes a route for `t` from `fromNode` to `t.destinationNodeId`,
    // keeping remainingRoute[0..1] untouched if keepCurrentEdge is true.
    bool rerouteFrom(ActiveTravel& t, int fromNode, bool keepCurrentEdge);

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
    int nextManualIncidentId_ = 100000;

    int currentTick_ = 0;
    size_t nextScenarioIndex_ = 0;
    static const int ON_SCENE_DURATION = 2;
};