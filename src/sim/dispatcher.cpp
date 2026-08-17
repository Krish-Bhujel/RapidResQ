#include "sim/dispatcher.hpp"
#include <limits>

Dispatcher::Dispatcher(Graph& g, IPathfinder& p, std::vector<Ambulance>& a)
    : graph(g), pathfinder(p), ambulances(a) {}

DispatchResult Dispatcher::assign(Incident& incident) {
    DispatchResult result;

    int bestIndex = -1;
    Path bestPath;
    double bestCost = std::numeric_limits<double>::max();

    for (int i = 0; i < ambulances.size(); i++) {
        if (ambulances[i].status != AmbulanceStatus::Idle) continue;

        Path candidate = pathfinder.findPath(graph, ambulances[i].currentNodeId, incident.nodeId);
        if (candidate.totalCost < 0) continue; // no route exists

        if (candidate.totalCost < bestCost) {
            bestCost = candidate.totalCost;
            bestIndex = i;
            bestPath = candidate;
        }
    }

    if (bestIndex == -1) {
        result.success = false;
        return result;
    }

    ambulances[bestIndex].status = AmbulanceStatus::EnRouteToIncident;
    incident.status = IncidentStatus::Assigned;
    incident.assignedAmbulanceId = ambulances[bestIndex].id;

    result.success = true;
    result.ambulanceId = ambulances[bestIndex].id;
    result.route = bestPath;
    return result;
}