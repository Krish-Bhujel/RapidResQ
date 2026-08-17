#include "ui/decoration.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool DecorationLoader::load(const std::string& path, std::vector<DecorationEntry>& outDecorations) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.size() == 0 || line[0] == '#') continue;

        std::istringstream iss(line);
        DecorationEntry entry;
        double sizeRaw = 0.0;
        if (iss >> entry.type >> entry.x >> entry.y) {
            if (iss >> sizeRaw && sizeRaw > 0.0) {
                entry.size = static_cast<float>(sizeRaw);
            } else {
                entry.size = -1.f;
            }
            outDecorations.push_back(entry);
        }
    }
    return true;
}