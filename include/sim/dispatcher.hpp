#pragma once
#include <vector>
#include "graph/graph.hpp"
#include "algo/ipathfinder.hpp"
#include "sim/ambulance.hpp"
#include "sim/incident.hpp"

struct DispatchResult {
    bool success = false;
    int ambulanceId = -1;
    Path route;
};

class Dispatcher {
public:
    Dispatcher(Graph& graph, IPathfinder& pathfinder, std::vector<Ambulance>& ambulances);
    DispatchResult assign(Incident& incident);

private:
    Graph& graph;
    IPathfinder& pathfinder;
    std::vector<Ambulance>& ambulances;
};