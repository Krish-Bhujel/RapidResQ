#pragma once
#include <vector>
#include "graph/graph.hpp"

struct Path {
    std::vector<int> nodes;
    double totalCost = -1.0;
};

class IPathfinder {
public:
    virtual ~IPathfinder() = default;
    virtual Path findPath(const Graph& graph, int startId, int endId) = 0;
};