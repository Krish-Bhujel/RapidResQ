#pragma once
#include <string>
#include "graph/graph.hpp"

// Reads intersections.txt and roads.txt, and fills up a Graph with them.
class MapLoader {
public:
    static bool load(Graph& graph, const std::string& nodesPath, const std::string& roadsPath);
};