#include "graph/graph.hpp"

void Graph::addNode(int id, double x, double y) {
    if (positions_.count(id)) {
        throw std::invalid_argument("Node with this id already exists: " + std::to_string(id));
    }
    positions_[id] = {x, y};
    adjacency_[id] = {};
}

void Graph::addEdge(int from, int to, double weight) {
    if (!positions_.count(from) || !positions_.count(to)) {
        throw std::out_of_range("Cannot add edge: unknown node id");
    }
    adjacency_[from].push_back({to, weight, false});
    adjacency_[to].push_back({from, weight, false});
}

std::vector<Edge> Graph::getNeighbours(int id) const {
    auto it = adjacency_.find(id);
    if (it == adjacency_.end()) {
        throw std::out_of_range("Unknown node id: " + std::to_string(id));
    }
    std::vector<Edge> result;
    for (const auto& e : it->second) {
        if (!e.blocked) {
            result.push_back({e.to, e.weight});
        }
    }
    return result;
}

void Graph::blockEdge(int from, int to) {
    auto setBlocked = [&](int a, int b, bool value) {
        auto it = adjacency_.find(a);
        if (it == adjacency_.end()) return;
        for (auto& e : it->second) {
            if (e.to == b) e.blocked = value;
        }
    };
    setBlocked(from, to, true);
    setBlocked(to, from, true);
}

void Graph::unblockEdge(int from, int to) {
    auto setBlocked = [&](int a, int b, bool value) {
        auto it = adjacency_.find(a);
        if (it == adjacency_.end()) return;
        for (auto& e : it->second) {
            if (e.to == b) e.blocked = value;
        }
    };
    setBlocked(from, to, false);
    setBlocked(to, from, false);
}

int Graph::nodeCount() const {
    return static_cast<int>(positions_.size());
}

NodePos Graph::getPosition(int id) const {
    auto it = positions_.find(id);
    if (it == positions_.end()) {
        throw std::out_of_range("Unknown node id: " + std::to_string(id));
    }
    return it->second;
}

bool Graph::hasNode(int id) const {
    return positions_.count(id) > 0;
}

std::vector<Graph::RenderEdge> Graph::getAllEdgesForRender() const {
    std::vector<RenderEdge> result;
    for (const auto& [nodeId, edges] : adjacency_) {
        for (const auto& e : edges) {
            if (nodeId < e.to) {
                result.push_back({nodeId, e.to, e.weight, e.blocked});
            }
        }
    }
    return result;
}

void Graph::clear() {
    positions_.clear();
    adjacency_.clear();
}

std::vector<int> Graph::getAllNodeIds() const {
    std::vector<int> ids;
    ids.reserve(positions_.size());
    for (const auto& [id, pos] : positions_) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<std::pair<int, int>> Graph::getBlockedEdges() const {
    std::vector<std::pair<int, int>> result;
    for (const auto& [nodeId, edges] : adjacency_) {
        for (const auto& e : edges) {
            if (e.blocked && nodeId < e.to) {
                result.push_back({nodeId, e.to});
            }
        }
    }
    return result;
}

bool Graph::isEdgeBlocked(int from, int to) const {
    auto it = adjacency_.find(from);
    if (it == adjacency_.end()) return false;
    for (const auto& e : it->second) {
        if (e.to == to) return e.blocked;
    }
    return false;
}

double Graph::getEdgeWeight(int from, int to) const {
    auto it = adjacency_.find(from);
    if (it == adjacency_.end()) return -1.0;
    for (const auto& e : it->second) {
        if (e.to == to) return e.weight;
    }
    return -1.0;
}

std::vector<std::pair<int, int>> Graph::getUnblockedEdges() const {
    std::vector<std::pair<int, int>> result;
    for (const auto& [nodeId, edges] : adjacency_) {
        for (const auto& e : edges) {
            if (!e.blocked && nodeId < e.to) {
                result.push_back({nodeId, e.to});
            }
        }
    }
    return result;
}