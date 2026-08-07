#include "ui/decoration.hpp"
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

bool DecorationLoader::load(const std::string& path, std::vector<DecorationEntry>& outDecorations) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "DecorationLoader: could not open " << path << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

        std::istringstream iss(line);
        DecorationEntry entry;
        double sizeRaw = 0.0;
        if (!(iss >> entry.type >> entry.x >> entry.y)) {
            std::cerr << "DecorationLoader: malformed line " << lineNumber << std::endl;
            continue;
        }
        if (iss >> sizeRaw && sizeRaw > 0.0) {
            entry.size = static_cast<float>(sizeRaw);
        } else {
            entry.size = -1.f;
        }
        outDecorations.push_back(entry);
    }
    return true;
}