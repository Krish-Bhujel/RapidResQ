#include "ui/map_renderer.hpp"
#include <limits>
#include <algorithm>
#include <cmath>
#include <iostream>

MapTransform::MapTransform(const Graph& graph, float sw, float sh, float pad)
    : padding_(pad), screenWidth_(sw), screenHeight_(sh) {

    double minXVal = std::numeric_limits<double>::max();
    double maxXVal = std::numeric_limits<double>::lowest();
    double minYVal = std::numeric_limits<double>::max();
    double maxYVal = std::numeric_limits<double>::lowest();

    std::vector<int> ids = graph.getAllNodeIds();
    for (int i = 0; i < ids.size(); i++) {
        NodePos p = graph.getPosition(ids[i]);
        if (p.x < minXVal) minXVal = p.x;
        if (p.x > maxXVal) maxXVal = p.x;
        if (p.y < minYVal) minYVal = p.y;
        if (p.y > maxYVal) maxYVal = p.y;
    }

    if (minXVal > maxXVal) {
        minXVal = 0; maxXVal = 1; minYVal = 0; maxYVal = 1;
    }

    double rangeX = maxXVal - minXVal;
    if (rangeX < 0.001) rangeX = 0.001;
    double rangeY = maxYVal - minYVal;
    if (rangeY < 0.001) rangeY = 0.001;

    double usableW = sw - 2 * pad;
    double usableH = sh - 2 * pad;

    double scaleX = usableW / rangeX;
    double scaleY = usableH / rangeY;
    scale_ = (scaleX < scaleY ? scaleX : scaleY) * 1.4;

    minX_ = minXVal;
    minY_ = minYVal;
}

sf::Vector2f MapTransform::toScreen(double x, double y) const {
    double centerX = 5.0;
    double centerY = 5.0;
    float sx = static_cast<float>((screenWidth_ / 2.f) + (x - centerX) * scale_);
    float sy = static_cast<float>((screenHeight_ / 2.f) - (y - centerY) * scale_);
    return sf::Vector2f(sx, sy);
}

MapRenderer::MapRenderer() {}

bool MapRenderer::loadFont(const std::string& path) {
    fontLoaded_ = font_.openFromFile(path);
    return fontLoaded_;
}

bool MapRenderer::loadAmbulanceIcon(const std::string& path) {
    ambulanceTextureLoaded_ = ambulanceTexture_.loadFromFile(path);
    return ambulanceTextureLoaded_;
}

bool MapRenderer::loadBrokenTreeIcon(const std::string& path) {
    brokenTreeTextureLoaded_ = brokenTreeTexture_.loadFromFile(path);
    return brokenTreeTextureLoaded_;
}

bool MapRenderer::loadDecorationIcon(const std::string& type, const std::string& path, float defaultSize) {
    sf::Texture tex;
    if (!tex.loadFromFile(path)) {
        std::cerr << "Failed to load decoration icon: " << type << std::endl;
        return false;
    }
    decorationTextures_[type] = tex;
    decorationDefaultSizes_[type] = defaultSize;
    return true;
}

void MapRenderer::setDecorations(const std::vector<DecorationEntry>& decos) {
    decorations_ = decos;
}

void MapRenderer::drawIconOrFallback(sf::RenderWindow& window, sf::Vector2f pos, float size,
                                      sf::Texture& texture, bool loaded, sf::Color fallbackColor,
                                      bool circleFallback, float rotationDegrees) {
    if (loaded) {
        sf::Sprite sprite(texture);
        sf::Vector2u texSize = texture.getSize();
        float maxSide = texSize.x > texSize.y ? static_cast<float>(texSize.x) : static_cast<float>(texSize.y);
        float scaleAmt = size / maxSide;
        sprite.setScale({scaleAmt, scaleAmt});
        sprite.setOrigin({texSize.x / 2.f, texSize.y / 2.f});
        sprite.setPosition(pos);
        sprite.setRotation(sf::degrees(rotationDegrees));
        window.draw(sprite);
    } else if (circleFallback) {
        sf::CircleShape c(size / 2.f);
        c.setFillColor(fallbackColor);
        c.setOrigin({size / 2.f, size / 2.f});
        c.setPosition(pos);
        window.draw(c);
    } else {
        sf::RectangleShape r({size, size});
        r.setFillColor(fallbackColor);
        r.setOrigin({size / 2.f, size / 2.f});
        r.setPosition(pos);
        r.setRotation(sf::degrees(rotationDegrees));
        window.draw(r);
    }
}

void MapRenderer::drawDecorations(sf::RenderWindow& window, const MapTransform& transform) {
    for (int i = 0; i < decorations_.size(); i++) {
        DecorationEntry d = decorations_[i];
        sf::Vector2f pos = transform.toScreen(d.x, d.y);

        bool loaded = decorationTextures_.count(d.type) > 0;

        float size = d.size;
        if (size <= 0.f) {
            if (decorationDefaultSizes_.count(d.type) > 0) {
                size = decorationDefaultSizes_[d.type];
            } else {
                size = DEFAULT_DECORATION_SIZE;
            }
        }

        if (loaded) {
            drawIconOrFallback(window, pos, size, decorationTextures_[d.type], true, sf::Color::White, false);
        } else {
            drawIconOrFallback(window, pos, size, ambulanceTexture_, false, sf::Color(220, 40, 220), false);
        }
    }
}

void MapRenderer::drawMap(sf::RenderWindow& window, const Graph& graph, const MapTransform& transform) {
    std::vector<int> ids = graph.getAllNodeIds();

    // Draw roads (avoid drawing each edge twice by only drawing when from < to)
    for (int i = 0; i < ids.size(); i++) {
        int from = ids[i];
        std::vector<Edge> neighbours = graph.getAllNeighbours(from);
        for (int j = 0; j < neighbours.size(); j++) {
            int to = neighbours[j].to;
            if (from >= to) continue;

            NodePos p1 = graph.getPosition(from);
            NodePos p2 = graph.getPosition(to);
            sf::Vector2f a = transform.toScreen(p1.x, p1.y);
            sf::Vector2f b = transform.toScreen(p2.x, p2.y);

            sf::Vector2f dir = b - a;
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            float angleDeg = std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;

            bool blocked = graph.isEdgeBlocked(from, to);

            sf::RectangleShape road({length, static_cast<float>(ROAD_WIDTH)});
            road.setOrigin({0.f, ROAD_WIDTH / 2.f});
            road.setPosition(a);
            road.setRotation(sf::degrees(angleDeg));
            if (blocked) {
                road.setFillColor(sf::Color(90, 40, 40));
            } else {
                road.setFillColor(sf::Color(70, 76, 86));
            }
            window.draw(road);

            sf::RectangleShape centerLine({length, 3.f});
            centerLine.setOrigin({0.f, 1.5f});
            centerLine.setPosition(a);
            centerLine.setRotation(sf::degrees(angleDeg));
            centerLine.setFillColor(sf::Color(220, 200, 120, 160));
            window.draw(centerLine);

            if (blocked) {
                sf::Vector2f mid = a + dir * 0.5f;
                drawIconOrFallback(window, mid, BROKEN_TREE_SIZE, brokenTreeTexture_,
                                    brokenTreeTextureLoaded_, sf::Color(110, 70, 40), false,
                                    angleDeg + 90.f);
            }
        }
    }

    // Draw nodes
    for (int i = 0; i < ids.size(); i++) {
        NodePos p = graph.getPosition(ids[i]);
        sf::Vector2f pos = transform.toScreen(p.x, p.y);

        sf::CircleShape circle(NODE_RADIUS);
        circle.setFillColor(sf::Color(235, 235, 240));
        circle.setOutlineColor(sf::Color(50, 55, 65));
        circle.setOutlineThickness(2.f);
        circle.setOrigin({static_cast<float>(NODE_RADIUS), static_cast<float>(NODE_RADIUS)});
        circle.setPosition(pos);
        window.draw(circle);

        if (fontLoaded_) {
            sf::Text label(font_);
            label.setString(std::to_string(ids[i]));
            label.setCharacterSize(14);
            label.setFillColor(sf::Color(235, 235, 240));
            label.setPosition({pos.x - 4.f, pos.y - 26.f});
            window.draw(label);
        }
    }
}

void MapRenderer::drawAmbulanceAt(sf::RenderWindow& window, sf::Vector2f pos, int ambulanceId, float rotationDegrees) {
    drawIconOrFallback(window, pos, AMBULANCE_SIZE, ambulanceTexture_, ambulanceTextureLoaded_,
                        sf::Color(220, 60, 60), true, rotationDegrees);

    if (fontLoaded_) {
        sf::Text label(font_);
        label.setString("A" + std::to_string(ambulanceId));
        label.setCharacterSize(12);
        label.setFillColor(sf::Color::White);
        label.setPosition({pos.x - 9.f, pos.y + AMBULANCE_SIZE / 2.f + 2.f});
        window.draw(label);
    }
}

void MapRenderer::drawAmbulanceRoute(sf::RenderWindow& window, const std::vector<int>& routeNodes,
                                      const Graph& graph, const MapTransform& transform, sf::Color color) {
    if (routeNodes.size() < 2) return;
    for (int i = 0; i + 1 < routeNodes.size(); i++) {
        NodePos p1 = graph.getPosition(routeNodes[i]);
        NodePos p2 = graph.getPosition(routeNodes[i + 1]);
        sf::Vector2f a = transform.toScreen(p1.x, p1.y);
        sf::Vector2f b = transform.toScreen(p2.x, p2.y);

        sf::Vector2f dir = b - a;
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        float angleDeg = std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;

        float overlayWidth = ROAD_WIDTH * 0.55f;
        sf::RectangleShape overlay({length, overlayWidth});
        overlay.setOrigin({0.f, overlayWidth / 2.f});
        overlay.setPosition(a);
        overlay.setRotation(sf::degrees(angleDeg));
        overlay.setFillColor(color);
        window.draw(overlay);
    }
}

void MapRenderer::drawIncidentBeacon(sf::RenderWindow& window, sf::Vector2f pos, float pulsePhase01) {
    float pulse = 0.5f + 0.5f * std::sin(pulsePhase01 * 3.14159265f * 2.f);
    float ringRadius = 14.f + pulse * 12.f;

    sf::CircleShape ring(ringRadius);
    ring.setOrigin({ringRadius, ringRadius});
    ring.setPosition(pos);
    ring.setFillColor(sf::Color(220, 40, 40, static_cast<std::uint8_t>(100.f * (1.f - pulse))));
    window.draw(ring);

    sf::CircleShape dot(7.f);
    dot.setOrigin({7.f, 7.f});
    dot.setPosition(pos);
    dot.setFillColor(sf::Color(255, 50, 50));
    window.draw(dot);
}

sf::Vector2f MapRenderer::interpolateAlongPath(const std::vector<int>& pathNodes, float progress,
                                                 const Graph& graph, const MapTransform& transform) {
    if (pathNodes.size() == 0) return sf::Vector2f(0.f, 0.f);
    if (pathNodes.size() == 1) {
        NodePos p = graph.getPosition(pathNodes[0]);
        return transform.toScreen(p.x, p.y);
    }

    if (progress < 0.f) progress = 0.f;
    if (progress > 1.f) progress = 1.f;

    int segmentCount = pathNodes.size() - 1;
    float scaled = progress * segmentCount;
    int segIndex = static_cast<int>(scaled);
    if (segIndex > segmentCount - 1) segIndex = segmentCount - 1;
    float segT = scaled - segIndex;

    NodePos p1 = graph.getPosition(pathNodes[segIndex]);
    NodePos p2 = graph.getPosition(pathNodes[segIndex + 1]);
    sf::Vector2f a = transform.toScreen(p1.x, p1.y);
    sf::Vector2f b = transform.toScreen(p2.x, p2.y);

    return sf::Vector2f(a.x + (b.x - a.x) * segT, a.y + (b.y - a.y) * segT);
}

float MapRenderer::getHeadingAlongPath(const std::vector<int>& pathNodes, float progress,
                                        const Graph& graph, const MapTransform& transform) {
    if (pathNodes.size() < 2) return 0.f;

    if (progress < 0.f) progress = 0.f;
    if (progress > 1.f) progress = 1.f;

    int segmentCount = pathNodes.size() - 1;
    float scaled = progress * segmentCount;
    int segIndex = static_cast<int>(scaled);
    if (segIndex > segmentCount - 1) segIndex = segmentCount - 1;

    NodePos p1 = graph.getPosition(pathNodes[segIndex]);
    NodePos p2 = graph.getPosition(pathNodes[segIndex + 1]);
    sf::Vector2f a = transform.toScreen(p1.x, p1.y);
    sf::Vector2f b = transform.toScreen(p2.x, p2.y);

    sf::Vector2f dir = b - a;
    return std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;
}