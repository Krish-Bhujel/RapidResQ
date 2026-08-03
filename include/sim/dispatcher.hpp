#pragma once
#include <vector>
#include "graph/graph.hpp"
#include "algo/ipathfinder.hpp"
#include "sim/ambulance.hpp"
#include "sim/hospital.hpp"
#include "sim/incident.hpp"

struct DispatchResult {
    bool success = false;
    int ambulanceId = -1;
    Path route;
};

class Dispatcher {
public:
    Dispatcher(Graph& graph, IPathfinder& pathfinder, std::vector<Ambulance>& ambulances);

    // Finds the best idle ambulance for this incident (lowest path cost),
    // marks it EnRouteToIncident, and returns the computed route.
    DispatchResult assign(Incident& incident);

private:
    Graph& graph_;
    IPathfinder& pathfinder_;
    std::vector<Ambulance>& ambulances_;
};