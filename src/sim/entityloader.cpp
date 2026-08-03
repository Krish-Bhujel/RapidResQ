#include "sim/entityloader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

static bool isBlankOrComment(const std::string& line) {
    for (char c : line) {
        if (c == '#') return true;
        if (!isspace(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

bool EntityLoader::loadHospitals(const std::string& path, std::vector<Hospital>& outHospitals) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "EntityLoader: could not open " << path << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

        std::istringstream iss(line);
        Hospital h;
        if (!(iss >> h.id >> h.nodeId >> h.capacity)) {
            std::cerr << "EntityLoader: malformed hospital line " << lineNumber << std::endl;
            continue;
        }
        outHospitals.push_back(h);
    }
    return true;
}

bool EntityLoader::loadAmbulances(const std::string& path, std::vector<Ambulance>& outAmbulances) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "EntityLoader: could not open " << path << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

        std::istringstream iss(line);
        Ambulance a;
        if (!(iss >> a.id >> a.homeHospitalId >> a.currentNodeId)) {
            std::cerr << "EntityLoader: malformed ambulance line " << lineNumber << std::endl;
            continue;
        }
        a.status = AmbulanceStatus::Idle;
        outAmbulances.push_back(a);
    }
    return true;
}