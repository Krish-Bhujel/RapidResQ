#include "algo/dijkstra.hpp"
#include <queue>
#include <unordered_map>
#include <algorithm>

Path Dijkstra::findPath(const Graph& graph, int startId, int endId) {
    std::priority_queue<std::pair<double, int>,
                         std::vector<std::pair<double, int>>,
                         std::greater<std::pair<double, int>>> pq;

    std::unordered_map<int, double> dist;
    std::unordered_map<int, int> prev;
    std::unordered_map<int, bool> visited;

    dist[startId] = 0.0;
    pq.push(std::make_pair(0.0, startId));

    while (!pq.empty()) {
        double d = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;
        if (u == endId) break;

        std::vector<Edge> neighbours = graph.getNeighbours(u);
        for (int i = 0; i < neighbours.size(); i++) {
            Edge e = neighbours[i];
            double newDist = d + e.weight;
            if (dist.count(e.to) == 0 || newDist < dist[e.to]) {
                dist[e.to] = newDist;
                prev[e.to] = u;
                pq.push(std::make_pair(newDist, e.to));
            }
        }
    }

    Path result;
    if (dist.count(endId) == 0) {
        result.totalCost = -1.0;
        return result;
    }

    std::vector<int> reversed;
    int current = endId;
    reversed.push_back(current);
    while (current != startId) {
        current = prev[current];
        reversed.push_back(current);
    }
    std::reverse(reversed.begin(), reversed.end());

    result.nodes = reversed;
    result.totalCost = dist[endId];
    return result;
}