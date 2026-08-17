#include "sim/scenarioloader.hpp"
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

bool ScenarioLoader::load(const std::string& path,
                           std::vector<Incident>& outIncidents,
                           std::vector<BlockEvent>& outBlocks) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ScenarioLoader: could not open " << path << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    int nextId = 1;
    while (std::getline(file, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "INCIDENT") {
            int tick, nodeId, sevRaw;
            if (!(iss >> tick >> nodeId >> sevRaw)) {
                std::cerr << "ScenarioLoader: malformed INCIDENT line " << lineNumber << std::endl;
                continue;
            }
            Incident inc;
            inc.id = nextId++;
            inc.triggerTick = tick;
            inc.nodeId = nodeId;
            inc.severity = static_cast<IncidentSeverity>(sevRaw);
            inc.status = IncidentStatus::Pending;
            outIncidents.push_back(inc);

        } else if (type == "BLOCK") {
            int tick, fromNode, toNode;
            if (!(iss >> tick >> fromNode >> toNode)) {
                std::cerr << "ScenarioLoader: malformed BLOCK line " << lineNumber << std::endl;
                continue;
            }
            outBlocks.push_back({tick, fromNode, toNode});

        } else {
            std::cerr << "ScenarioLoader: unknown line type '" << type
                       << "' at line " << lineNumber << std::endl;
        }
    }
    return true;
}