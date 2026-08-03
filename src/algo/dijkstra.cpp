#include "algo/dijkstra.hpp"
#include <queue>
#include <unordered_map>
#include <limits>
#include <algorithm>

Path Dijkstra::findPath(const Graph& graph, int startId, int endId) {
    // min-heap of (distance, nodeId), smallest distance first
    using PQItem = std::pair<double, int>;
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> pq;

    std::unordered_map<int, double> dist;
    std::unordered_map<int, int> prev;
    std::unordered_map<int, bool> visited;

    dist[startId] = 0.0;
    pq.push({0.0, startId});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;

        if (u == endId) break; // shortest path to end found

        for (const Edge& e : graph.getNeighbours(u)) {
            double newDist = d + e.weight;
            auto it = dist.find(e.to);
            if (it == dist.end() || newDist < it->second) {
                dist[e.to] = newDist;
                prev[e.to] = u;
                pq.push({newDist, e.to});
            }
        }
    }

    Path result;
    if (!dist.count(endId)) {
        result.totalCost = -1.0; // no path exists
        return result;
    }

    // reconstruct path by walking backwards through prev[]
    std::vector<int> reversed;
    int current = endId;
    reversed.push_back(current);
    while (current != startId) {
        current = prev.at(current);
        reversed.push_back(current);
    }
    std::reverse(reversed.begin(), reversed.end());

    result.nodes = reversed;
    result.totalCost = dist[endId];
    return result;
}