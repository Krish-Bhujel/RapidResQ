#pragma once
#include <vector>
#include <unordered_map>
#include <stdexcept>

// One road connecting to a neighbouring intersection.
struct Edge {
    int to;
    double weight;
    bool blocked = false;
};

// The (x, y) position of an intersection, used for drawing on screen.
struct NodePos {
    double x;
    double y;
};

// Represents the whole city map: intersections (nodes) and roads (edges).
class Graph {
public:
    void addNode(int id, double x, double y);
    void addEdge(int from, int to, double weight);

    std::vector<Edge> getNeighbours(int id) const;
    std::vector<Edge> getAllNeighbours(int id) const;
    bool isEdgeBlocked(int from, int to) const;
    double getEdgeWeight(int from, int to) const;

    void blockEdge(int from, int to);
    void unblockEdge(int from, int to);

    bool hasNode(int id) const;
    NodePos getPosition(int id) const;
    int nodeCount() const;
    std::vector<int> getAllNodeIds() const;

    void clear();

private:
    std::unordered_map<int, NodePos> positions;
    std::unordered_map<int, std::vector<Edge>> adjacency;
};