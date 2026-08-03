#pragma once
#include <string>
#include "graph/graph.hpp"

class MapLoader {
public:
    // Loads nodes from intersectionsPath and edges from roadsPath into the given graph.
    // Returns true on success, false if either file couldn't be opened.
    static bool load(Graph& graph, const std::string& intersectionsPath, const std::string& roadsPath);
};