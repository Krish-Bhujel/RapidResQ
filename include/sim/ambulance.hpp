#pragma once

enum class AmbulanceStatus {
    Idle,
    EnRouteToIncident,
    OnScene,
    ReturningToHospital
};

struct Ambulance {
    int id;
    int currentNodeId;
    int homeHospitalId;
    AmbulanceStatus status = AmbulanceStatus::Idle;
};