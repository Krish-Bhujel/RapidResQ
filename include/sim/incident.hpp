#pragma once

enum class IncidentSeverity { Low, Medium, High };
enum class IncidentStatus { Pending, Assigned, Resolved };

struct Incident {
    int id;
    int nodeId;
    int triggerTick = 0;
    IncidentSeverity severity = IncidentSeverity::Medium;
    IncidentStatus status = IncidentStatus::Pending;
    int assignedAmbulanceId = -1;
};