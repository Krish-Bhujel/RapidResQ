#include "graph/maploader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool MapLoader::load(Graph& graph, const std::string& nodesPath, const std::string& roadsPath) {

    // ---- Step 1: read the intersections file ----
    std::ifstream nodesFile(nodesPath);
    if (!nodesFile.is_open()) {
        std::cerr << "Could not open " << nodesPath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(nodesFile, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue; // skip empty lines and comments
        }

        std::istringstream iss(line);
        int id;
        double x, y;
        if (iss >> id >> x >> y) {
            try {
                graph.addNode(id, x, y);
            } catch (const std::exception& e) {
                std::cerr << "Skipping bad node line: " << e.what() << std::endl;
            }
        }
    }
    nodesFile.close();

    // ---- Step 2: read the roads file ----
    std::ifstream roadsFile(roadsPath);
    if (!roadsFile.is_open()) {
        std::cerr << "Could not open " << roadsPath << std::endl;
        return false;
    }

    while (std::getline(roadsFile, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        int from, to;
        double weight;
        if (iss >> from >> to >> weight) {
            try {
                graph.addEdge(from, to, weight);
            } catch (const std::exception& e) {
                std::cerr << "Skipping bad road line: " << e.what() << std::endl;
            }
        }
    }
    roadsFile.close();

    return true;
}