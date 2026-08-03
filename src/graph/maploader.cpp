#include "graph/maploader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

static bool isBlankOrComment(const std::string& line) {
    for (char c : line) {
        if (c == '#') return true;
        if (!isspace(static_cast<unsigned char>(c))) return false;
    }
    return true; // all whitespace
}

bool MapLoader::load(Graph& graph, const std::string& intersectionsPath, const std::string& roadsPath) {
    std::ifstream nodesFile(intersectionsPath);
    if (!nodesFile.is_open()) {
        std::cerr << "MapLoader: could not open " << intersectionsPath << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(nodesFile, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

        std::istringstream iss(line);
        int id;
        double x, y;
        if (!(iss >> id >> x >> y)) {
            std::cerr << "MapLoader: malformed line " << lineNumber
                      << " in " << intersectionsPath << ": \"" << line << "\"" << std::endl;
            continue;
        }
        try {
            graph.addNode(id, x, y);
        } catch (const std::exception& e) {
            std::cerr << "MapLoader: " << e.what() << " (line " << lineNumber << ")" << std::endl;
        }
    }

    std::ifstream edgesFile(roadsPath);
    if (!edgesFile.is_open()) {
        std::cerr << "MapLoader: could not open " << roadsPath << std::endl;
        return false;
    }

    lineNumber = 0;
    while (std::getline(edgesFile, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

        std::istringstream iss(line);
        int from, to;
        double weight;
        if (!(iss >> from >> to >> weight)) {
            std::cerr << "MapLoader: malformed line " << lineNumber
                      << " in " << roadsPath << ": \"" << line << "\"" << std::endl;
            continue;
        }
        try {
            graph.addEdge(from, to, weight);
        } catch (const std::exception& e) {
            std::cerr << "MapLoader: " << e.what() << " (line " << lineNumber << ")" << std::endl;
        }
    }

    return true;
}