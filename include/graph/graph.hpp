#pragma once
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <utility>

struct Edge {
    int to;
    double weight;
};

struct NodePos {
    double x;
    double y;
};

class Graph {
public:
    void addNode(int id, double x, double y);
    void addEdge(int from, int to, double weight);
    std::vector<Edge> getNeighbours(int id) const;
    void blockEdge(int from, int to);
    void unblockEdge(int from, int to);
    int nodeCount() const;
    NodePos getPosition(int id) const;
    bool hasNode(int id) const;

    struct RenderEdge {
        int from;
        int to;
        double weight;
        bool blocked;
    };
    std::vector<RenderEdge> getAllEdgesForRender() const;

    // Wipes the graph completely so it can be reloaded from scratch.
    void clear();

    // All node ids currently in the graph, in no particular order.
    std::vector<int> getAllNodeIds() const;

    // Every currently blocked edge, each listed once (from < to).
    std::vector<std::pair<int, int>> getBlockedEdges() const;

private:
    struct StoredEdge {
        int to;
        double weight;
        bool blocked = false;
    };

    std::unordered_map<int, NodePos> positions_;
    std::unordered_map<int, std::vector<StoredEdge>> adjacency_;
};