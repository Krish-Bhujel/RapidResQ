#pragma once
#include <string>
#include <vector>
#include "sim/ambulance.hpp"
#include "sim/hospital.hpp"

class EntityLoader {
public:
    static bool loadHospitals(const std::string& path, std::vector<Hospital>& outHospitals);
    static bool loadAmbulances(const std::string& path, std::vector<Ambulance>& outAmbulances);
};