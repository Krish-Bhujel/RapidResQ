#include "ui/map_renderer.hpp"
#include <limits>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace std;

// ---------------------------------------------------------------------
// MapTransform  (kept exactly as you customized it)
// ---------------------------------------------------------------------

MapTransform::MapTransform(const Graph& graph, float screenWidth, float screenHeight, float padding)
    : padding_(padding),
      screenWidth_(screenWidth),
      screenHeight_(screenHeight) {

    double minX =  numeric_limits<double>::max();
    double maxX =  numeric_limits<double>::lowest();
    double minY =  numeric_limits<double>::max();
    double maxY =  numeric_limits<double>::lowest();

    for (int id = 1; id <= 2000; ++id) {
        if (!graph.hasNode(id)) continue;
        NodePos p = graph.getPosition(id);
        minX =  min(minX, p.x);
        maxX =  max(maxX, p.x);
        minY =  min(minY, p.y);
        maxY =  max(maxY, p.y);
    }

    if (minX > maxX) { minX = 0; maxX = 1; minY = 0; maxY = 1; }

    double rangeX =  max(0.001, maxX - minX);
    double rangeY =  max(0.001, maxY - minY);

    double usableW = screenWidth - 2 * padding;
    double usableH = screenHeight - 2 * padding;

    scale_ =  min(usableW / rangeX, usableH / rangeY) * 1.3;
    minX_ = minX;
    minY_ = minY;
}

sf::Vector2f MapTransform::toScreen(double x, double y) const {
    const double centerX = 5.0;
    const double centerY = 5.0;

    float sx = static_cast<float>((screenWidth_ / 2.f) + (x - centerX) * scale_);
    float sy = static_cast<float>((screenHeight_ / 2.f) - (y - centerY) * scale_);
    return {sx, sy};
}

// ---------------------------------------------------------------------
// MapRenderer
// ---------------------------------------------------------------------

MapRenderer::MapRenderer() {}

bool MapRenderer::loadFont(const  string& fontPath) {
    fontLoaded_ = font_.openFromFile(fontPath);
    return fontLoaded_;
}
bool MapRenderer::loadAmbulanceIcon(const  string& path) {
    ambulanceTextureLoaded_ = ambulanceTexture_.loadFromFile(path);
    return ambulanceTextureLoaded_;
}
bool MapRenderer::loadHospitalIcon(const  string& path) {
    hospitalTextureLoaded_ = hospitalTexture_.loadFromFile(path);
    return hospitalTextureLoaded_;
}
bool MapRenderer::loadBrokenTreeIcon(const  string& path) {
    brokenTreeTextureLoaded_ = brokenTreeTexture_.loadFromFile(path);
    return brokenTreeTextureLoaded_;
}

bool MapRenderer::loadDecorationIcon(const  string& type, const  string& path, float defaultSize) {
    sf::Texture tex;
    if (!tex.loadFromFile(path)) {
         cerr << "MapRenderer: failed to load decoration icon '" << type << "' from " << path <<  endl;
        return false;
    }
    decorationTextures_[type] =  move(tex);
    decorationDefaultSizes_[type] = defaultSize; // <-- per-type size lives here
    return true;
}

void MapRenderer::setDecorations(const  vector<DecorationEntry>& decorations) {
    decorations_ = decorations;
}

void MapRenderer::drawIconOrFallback(sf::RenderWindow& window, sf::Vector2f pos, float targetSize,
                                      sf::Texture& texture, bool loaded, sf::Color fallbackColor,
                                      bool fallbackIsCircle, float rotationDegrees) {
    if (loaded) {
        sf::Sprite sprite(texture);
        sf::Vector2u texSize = texture.getSize();
        float scale = targetSize / static_cast<float>( max(texSize.x, texSize.y));
        sprite.setScale({scale, scale});
        sprite.setOrigin({texSize.x / 2.f, texSize.y / 2.f});
        sprite.setPosition(pos);
        sprite.setRotation(sf::degrees(rotationDegrees));
        window.draw(sprite);
    } else if (fallbackIsCircle) {
        sf::CircleShape c(targetSize / 2.f);
        c.setFillColor(fallbackColor);
        c.setOrigin({targetSize / 2.f, targetSize / 2.f});
        c.setPosition(pos);
        window.draw(c);
    } else {
        sf::RectangleShape r({targetSize, targetSize});
        r.setFillColor(fallbackColor);
        r.setOrigin({targetSize / 2.f, targetSize / 2.f});
        r.setPosition(pos);
        r.setRotation(sf::degrees(rotationDegrees));
        window.draw(r);
    }
}

void MapRenderer::drawDecorations(sf::RenderWindow& window, const MapTransform& transform) {
    for (const auto& d : decorations_) {
        sf::Vector2f pos = transform.toScreen(d.x, d.y);

        auto texIt = decorationTextures_.find(d.type);
        bool loaded = (texIt != decorationTextures_.end());

        float size = d.size; // per-entry override, if set
        if (size <= 0.f) {
            auto sizeIt = decorationDefaultSizes_.find(d.type);
            size = (sizeIt != decorationDefaultSizes_.end()) ? sizeIt->second : DEFAULT_DECORATION_SIZE;
        }

        if (loaded) {
            drawIconOrFallback(window, pos, size, texIt->second, true, sf::Color::White, false);
        } else {
            // Unregistered type: draw a magenta square so it's obvious something's misconfigured,
            // instead of silently not showing anything.
            drawIconOrFallback(window, pos, size, ambulanceTexture_, false, sf::Color(220, 40, 220), false);
        }
    }
}

void MapRenderer::drawMap(sf::RenderWindow& window, const Graph& graph, const MapTransform& transform) {
    for (const auto& e : graph.getAllEdgesForRender()) {
        NodePos p1 = graph.getPosition(e.from);
        NodePos p2 = graph.getPosition(e.to);
        sf::Vector2f a = transform.toScreen(p1.x, p1.y);
        sf::Vector2f b = transform.toScreen(p2.x, p2.y);

        sf::Vector2f dir = b - a;
        float length =  sqrt(dir.x * dir.x + dir.y * dir.y);
        float angleDeg =  atan2(dir.y, dir.x) * 180.f / 3.14159265f;

        sf::RectangleShape road({length, ROAD_WIDTH});
        road.setOrigin({0.f, ROAD_WIDTH / 2.f});
        road.setPosition(a);
        road.setRotation(sf::degrees(angleDeg));
        road.setFillColor(e.blocked ? sf::Color(90, 40, 40) : sf::Color(70, 76, 86));
        window.draw(road);

        sf::RectangleShape centerLine({length, 3.f});
        centerLine.setOrigin({0.f, 1.5f});
        centerLine.setPosition(a);
        centerLine.setRotation(sf::degrees(angleDeg));
        centerLine.setFillColor(sf::Color(220, 200, 120, 160));
        window.draw(centerLine);

        if (e.blocked) {
            sf::Vector2f mid = a + dir * 0.5f;
            drawIconOrFallback(window, mid, BROKEN_TREE_SIZE, brokenTreeTexture_,
                                brokenTreeTextureLoaded_, sf::Color(110, 70, 40), false,
                                angleDeg + 90.f);
        }
    }

    for (int id = 1; id <= 2000; ++id) {
        if (!graph.hasNode(id)) continue;
        NodePos p = graph.getPosition(id);
        sf::Vector2f pos = transform.toScreen(p.x, p.y);

        sf::CircleShape circle(NODE_RADIUS);
        circle.setFillColor(sf::Color(235, 235, 240));
        circle.setOutlineColor(sf::Color(50, 55, 65));
        circle.setOutlineThickness(2.f);
        circle.setOrigin({NODE_RADIUS, NODE_RADIUS});
        circle.setPosition(pos);
        window.draw(circle);

        if (fontLoaded_) {
            sf::Text label(font_);
            label.setString( to_string(id));
            label.setCharacterSize(14);
            label.setFillColor(sf::Color(235, 235, 240));
            label.setPosition({pos.x - 4.f, pos.y - 26.f});
            window.draw(label);
        }
    }
}

void MapRenderer::drawHospitals(sf::RenderWindow& window, const  vector<Hospital>& hospitals,
                                 const Graph& graph, const MapTransform& transform) {
    const float HOSPITAL_OFFSET_Y = -70.f;

    for (const auto& h : hospitals) {
        if (!graph.hasNode(h.nodeId)) continue;
        NodePos p = graph.getPosition(h.nodeId);
        sf::Vector2f nodePos = transform.toScreen(p.x, p.y);
        sf::Vector2f pos = { nodePos.x, nodePos.y + HOSPITAL_OFFSET_Y };

        sf::Vertex line[] = {
            sf::Vertex{nodePos, sf::Color(60, 130, 220, 140)},
            sf::Vertex{pos, sf::Color(60, 130, 220, 140)}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);

        drawIconOrFallback(window, pos, HOSPITAL_SIZE, hospitalTexture_, hospitalTextureLoaded_,
                            sf::Color(60, 130, 220), false);

        if (fontLoaded_) {
            sf::Text label(font_);
            label.setString("H" +  to_string(h.id));
            label.setCharacterSize(14);
            label.setFillColor(sf::Color::White);
            label.setPosition({pos.x - 10.f, pos.y - HOSPITAL_SIZE / 2.f - 20.f});
            window.draw(label);
        }
    }
}

void MapRenderer::drawAmbulanceAt(sf::RenderWindow& window, sf::Vector2f screenPos, int ambulanceId,
                                   float rotationDegrees) {
    drawIconOrFallback(window, screenPos, AMBULANCE_SIZE, ambulanceTexture_, ambulanceTextureLoaded_,
                        sf::Color(220, 60, 60), true, rotationDegrees);

    if (fontLoaded_) {
        sf::Text label(font_);
        label.setString("A" +  to_string(ambulanceId));
        label.setCharacterSize(12);
        label.setFillColor(sf::Color::White);
        label.setPosition({screenPos.x - 9.f, screenPos.y + AMBULANCE_SIZE / 2.f + 2.f});
        window.draw(label);
    }
}

sf::Vector2f MapRenderer::interpolateAlongPath(const  vector<int>& pathNodes, float progress,
                                                 const Graph& graph, const MapTransform& transform) {
    if (pathNodes.empty()) return {0.f, 0.f};
    if (pathNodes.size() == 1) {
        NodePos p = graph.getPosition(pathNodes[0]);
        return transform.toScreen(p.x, p.y);
    }

    progress =  max(0.f,  min(1.f, progress));

    int segmentCount = static_cast<int>(pathNodes.size()) - 1;
    float scaledProgress = progress * segmentCount;
    int segmentIndex =  min(segmentCount - 1, static_cast<int>(scaledProgress));
    float segmentT = scaledProgress - segmentIndex;

    NodePos p1 = graph.getPosition(pathNodes[segmentIndex]);
    NodePos p2 = graph.getPosition(pathNodes[segmentIndex + 1]);
    sf::Vector2f a = transform.toScreen(p1.x, p1.y);
    sf::Vector2f b = transform.toScreen(p2.x, p2.y);

    return {
        a.x + (b.x - a.x) * segmentT,
        a.y + (b.y - a.y) * segmentT
    };
}

float MapRenderer::getHeadingAlongPath(const  vector<int>& pathNodes, float progress,
                                        const Graph& graph, const MapTransform& transform) {
    if (pathNodes.size() < 2) return 0.f;

    progress =  max(0.f,  min(1.f, progress));

    int segmentCount = static_cast<int>(pathNodes.size()) - 1;
    float scaledProgress = progress * segmentCount;
    int segmentIndex =  min(segmentCount - 1, static_cast<int>(scaledProgress));

    NodePos p1 = graph.getPosition(pathNodes[segmentIndex]);
    NodePos p2 = graph.getPosition(pathNodes[segmentIndex + 1]);
    sf::Vector2f a = transform.toScreen(p1.x, p1.y);
    sf::Vector2f b = transform.toScreen(p2.x, p2.y);

    sf::Vector2f dir = b - a;
    return  atan2(dir.y, dir.x) * 180.f / 3.14159265f;
}

void MapRenderer::drawAmbulanceRoute(sf::RenderWindow& window, const  vector<int>& routeNodes,
                                      const Graph& graph, const MapTransform& transform, sf::Color color) {
    if (routeNodes.size() < 2) return;
    for (size_t i = 0; i + 1 < routeNodes.size(); ++i) {
        NodePos p1 = graph.getPosition(routeNodes[i]);
        NodePos p2 = graph.getPosition(routeNodes[i + 1]);
        sf::Vector2f a = transform.toScreen(p1.x, p1.y);
        sf::Vector2f b = transform.toScreen(p2.x, p2.y);

        sf::Vector2f dir = b - a;
        float length =  sqrt(dir.x * dir.x + dir.y * dir.y);
        float angleDeg =  atan2(dir.y, dir.x) * 180.f / 3.14159265f;

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
    float pulse = 0.5f + 0.5f *  sin(pulsePhase01 * 3.14159265f * 2.f);
    float ringRadius = 14.f + pulse * 12.f;

    sf::CircleShape ring(ringRadius);
    ring.setOrigin({ringRadius, ringRadius});
    ring.setPosition(pos);
    ring.setFillColor(sf::Color(220, 40, 40, static_cast< uint8_t>(100.f * (1.f - pulse))));
    window.draw(ring);

    sf::CircleShape dot(7.f);
    dot.setOrigin({7.f, 7.f});
    dot.setPosition(pos);
    dot.setFillColor(sf::Color(255, 50, 50));
    window.draw(dot);
}