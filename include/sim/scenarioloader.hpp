#pragma once
#include <string>
#include <vector>
#include "sim/incident.hpp"
#include "sim/block_event.hpp"

class ScenarioLoader {
public:
    static bool load(const std::string& path,
                      std::vector<Incident>& outIncidents,
                      std::vector<BlockEvent>& outBlocks);
};