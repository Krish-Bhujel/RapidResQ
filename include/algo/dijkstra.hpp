#pragma once
#include "algo/ipathfinder.hpp"

class Dijkstra : public IPathfinder {
public:
    Path findPath(const Graph& graph, int startId, int endId) override;
};