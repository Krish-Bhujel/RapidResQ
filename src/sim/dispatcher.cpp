#include "sim/dispatcher.hpp"
#include <limits>

Dispatcher::Dispatcher(Graph& graph, IPathfinder& pathfinder, std::vector<Ambulance>& ambulances)
    : graph_(graph), pathfinder_(pathfinder), ambulances_(ambulances) {}

DispatchResult Dispatcher::assign(Incident& incident) {
    DispatchResult result;

    Ambulance* best = nullptr;
    Path bestPath;
    double bestCost = std::numeric_limits<double>::max();

    for (auto& amb : ambulances_) {
        if (amb.status != AmbulanceStatus::Idle) continue;

        Path candidate = pathfinder_.findPath(graph_, amb.currentNodeId, incident.nodeId);
        if (candidate.totalCost < 0) continue; // no path to this incident

        if (candidate.totalCost < bestCost) {
            bestCost = candidate.totalCost;
            best = &amb;
            bestPath = candidate;
        }
    }

    if (!best) {
        result.success = false;
        return result;
    }

    best->status = AmbulanceStatus::EnRouteToIncident;
    incident.status = IncidentStatus::Assigned;
    incident.assignedAmbulanceId = best->id;

    result.success = true;
    result.ambulanceId = best->id;
    result.route = bestPath;
    return result;
}