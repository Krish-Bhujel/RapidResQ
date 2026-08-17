#include "sim/entityloader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool EntityLoader::loadHospitals(const std::string& path, std::vector<Hospital>& outHospitals) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        Hospital h;
        if (iss >> h.id >> h.nodeId >> h.capacity) {
            outHospitals.push_back(h);
        }
    }
    return true;
}

bool EntityLoader::loadAmbulances(const std::string& path, std::vector<Ambulance>& outAmbulances) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        Ambulance a;
        if (iss >> a.id >> a.homeHospitalId >> a.currentNodeId) {
            a.status = AmbulanceStatus::Idle;
            outAmbulances.push_back(a);
        }
    }
    return true;
}