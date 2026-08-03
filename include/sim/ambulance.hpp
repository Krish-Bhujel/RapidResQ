#pragma once

enum class AmbulanceStatus {
    Idle,
    EnRouteToIncident,
    OnScene,
    ReturningToHospital
};

struct Ambulance {
    int id;
    int currentNodeId;   // where it is right now on the graph
    int homeHospitalId;  // which hospital it's based at
    AmbulanceStatus status = AmbulanceStatus::Idle;
};