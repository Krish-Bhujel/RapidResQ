#include "sim/scenarioloader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool ScenarioLoader::load(const std::string& path,
                           std::vector<Incident>& outIncidents,
                           std::vector<BlockEvent>& outBlocks) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        return false;
    }

    std::string line;
    int nextId = 1;
    while (std::getline(file, line)) {
        if (line.size() == 0 || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "INCIDENT") {
            int tick, nodeId, sevRaw;
            if (iss >> tick >> nodeId >> sevRaw) {
                Incident inc;
                inc.id = nextId;
                nextId++;
                inc.triggerTick = tick;
                inc.nodeId = nodeId;
                inc.severity = static_cast<IncidentSeverity>(sevRaw);
                outIncidents.push_back(inc);
            }
        } else if (type == "BLOCK") {
            int tick, fromNode, toNode;
            if (iss >> tick >> fromNode >> toNode) {
                BlockEvent b;
                b.tick = tick;
                b.fromNode = fromNode;
                b.toNode = toNode;
                outBlocks.push_back(b);
            }
        }
    }
    return true;
}