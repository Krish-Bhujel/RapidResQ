#include "graph/graph.hpp"

void Graph::addNode(int id, double x, double y) {
    if (positions.count(id) > 0) {
        // Node already exists — throw stops the program with an error message
        // if this ever happens, instead of silently corrupting data.
        throw std::invalid_argument("Node already exists");
    }
    positions[id] = NodePos{x, y};
    adjacency[id] = std::vector<Edge>(); // start with an empty road list
}

void Graph::addEdge(int from, int to, double weight) {
    if (positions.count(from) == 0 || positions.count(to) == 0) {
        throw std::out_of_range("Unknown node id");
    }
    // Roads work both directions, so we add the edge twice.
    Edge e1;
    e1.to = to;
    e1.weight = weight;
    adjacency[from].push_back(e1);

    Edge e2;
    e2.to = from;
    e2.weight = weight;
    adjacency[to].push_back(e2);
}

std::vector<Edge> Graph::getNeighbours(int id) const {
    std::vector<Edge> result;
    if (adjacency.count(id) == 0) {
        throw std::out_of_range("Unknown node id");
    }
    for (int i = 0; i < adjacency.at(id).size(); i++) {
        Edge e = adjacency.at(id)[i];
        if (!e.blocked) {
            result.push_back(e);
        }
    }
    return result;
}

std::vector<Edge> Graph::getAllNeighbours(int id) const {
    if (adjacency.count(id) == 0) {
        throw std::out_of_range("Unknown node id");
    }

    return adjacency.at(id);
}

bool Graph::isEdgeBlocked(int from, int to) const {
    if (adjacency.count(from) == 0) return false;
    for (int i = 0; i < adjacency.at(from).size(); i++) {
        if (adjacency.at(from)[i].to == to) {
            return adjacency.at(from)[i].blocked;
        }
    }
    return false;
}

double Graph::getEdgeWeight(int from, int to) const {
    if (adjacency.count(from) == 0) return -1;
    for (int i = 0; i < adjacency.at(from).size(); i++) {
        if (adjacency.at(from)[i].to == to) {
            return adjacency.at(from)[i].weight;
        }
    }
    return -1;
}

void Graph::blockEdge(int from, int to) {
    if (adjacency.count(from) > 0) {
        for (int i = 0; i < adjacency[from].size(); i++) {
            if (adjacency[from][i].to == to) {
                adjacency[from][i].blocked = true;
            }
        }
    }
    if (adjacency.count(to) > 0) {
        for (int i = 0; i < adjacency[to].size(); i++) {
            if (adjacency[to][i].to == from) {
                adjacency[to][i].blocked = true;
            }
        }
    }
}

void Graph::unblockEdge(int from, int to) {
    if (adjacency.count(from) > 0) {
        for (int i = 0; i < adjacency[from].size(); i++) {
            if (adjacency[from][i].to == to) {
                adjacency[from][i].blocked = false;
            }
        }
    }
    if (adjacency.count(to) > 0) {
        for (int i = 0; i < adjacency[to].size(); i++) {
            if (adjacency[to][i].to == from) {
                adjacency[to][i].blocked = false;
            }
        }
    }
}

bool Graph::hasNode(int id) const {
    return positions.count(id) > 0;
}

NodePos Graph::getPosition(int id) const {
    if (positions.count(id) == 0) {
        throw std::out_of_range("Unknown node id");
    }
    return positions.at(id);
}

int Graph::nodeCount() const {
    return positions.size();
}

std::vector<int> Graph::getAllNodeIds() const {
    std::vector<int> ids;
    for (auto pair : positions) {
        ids.push_back(pair.first);
    }
    return ids;
}

void Graph::clear() {
    positions.clear();
    adjacency.clear();
}