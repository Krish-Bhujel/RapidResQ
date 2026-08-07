#include "ui/map_renderer.hpp"
#include <limits>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------
// MapTransform
// ---------------------------------------------------------------------

MapTransform::MapTransform(const Graph &graph, float screenWidth, float screenHeight, float padding)
    : padding_(padding),
      screenWidth_(screenWidth),
      screenHeight_(screenHeight)
{

    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (int id = 1; id <= 2000; ++id)
    {
        if (!graph.hasNode(id))
            continue;
        NodePos p = graph.getPosition(id);
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    if (minX > maxX)
    {
        minX = 0;
        maxX = 1;
        minY = 0;
        maxY = 1;
    }

    double rangeX = std::max(0.001, maxX - minX);
    double rangeY = std::max(0.001, maxY - minY);

    double usableW = screenWidth - 2 * padding;
    double usableH = screenHeight - 2 * padding;

    scale_ = std::min(usableW / rangeX, usableH / rangeY)*1.4;
    minX_ = minX;
    minY_ = minY;
}

sf::Vector2f MapTransform::toScreen(double x, double y) const
{
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

bool MapRenderer::loadFont(const std::string &fontPath)
{
    fontLoaded_ = font_.openFromFile(fontPath);
    return fontLoaded_;
}
bool MapRenderer::loadAmbulanceIcon(const std::string &path)
{
    ambulanceTextureLoaded_ = ambulanceTexture_.loadFromFile(path);
    return ambulanceTextureLoaded_;
}
bool MapRenderer::loadHospitalIcon(const std::string &path)
{
    hospitalTextureLoaded_ = hospitalTexture_.loadFromFile(path);
    return hospitalTextureLoaded_;
}
bool MapRenderer::loadTreeIcon(const std::string &path)
{
    treeTextureLoaded_ = treeTexture_.loadFromFile(path);
    return treeTextureLoaded_;
}
bool MapRenderer::loadHouseIcon(const std::string &path)
{
    houseTextureLoaded_ = houseTexture_.loadFromFile(path);
    return houseTextureLoaded_;
}
bool MapRenderer::loadBrokenTreeIcon(const std::string &path)
{
    brokenTreeTextureLoaded_ = brokenTreeTexture_.loadFromFile(path);
    return brokenTreeTextureLoaded_;
}
bool MapRenderer::loadFountainIcon(const std::string &path)
{
    fountainTextureLoaded_ = fountainTexture_.loadFromFile(path);
    return fountainTextureLoaded_;
}

void MapRenderer::drawIconOrFallback(sf::RenderWindow &window, sf::Vector2f pos, float targetSize,
                                     sf::Texture &texture, bool loaded, sf::Color fallbackColor,
                                     bool fallbackIsCircle, float rotationDegrees)
{
    if (loaded)
    {
        sf::Sprite sprite(texture);
        sf::Vector2u texSize = texture.getSize();
        float scale = targetSize / static_cast<float>(std::max(texSize.x, texSize.y));
        sprite.setScale({scale, scale});
        sprite.setOrigin({texSize.x / 2.f, texSize.y / 2.f});
        sprite.setPosition(pos);
        sprite.setRotation(sf::degrees(rotationDegrees));
        window.draw(sprite);
    }
    else if (fallbackIsCircle)
    {
        sf::CircleShape c(targetSize / 2.f);
        c.setFillColor(fallbackColor);
        c.setOrigin({targetSize / 2.f, targetSize / 2.f});
        c.setPosition(pos);
        window.draw(c);
    }
    else
    {
        sf::RectangleShape r({targetSize, targetSize});
        r.setFillColor(fallbackColor);
        r.setOrigin({targetSize / 2.f, targetSize / 2.f});
        r.setPosition(pos);
        r.setRotation(sf::degrees(rotationDegrees));
        window.draw(r);
    }
}

void MapRenderer::generateDecorations(const Graph &graph, const MapTransform &transform)
{
    decorations_.clear();

    for (const auto &e : graph.getAllEdgesForRender())
    {
        NodePos p1 = graph.getPosition(e.from);
        NodePos p2 = graph.getPosition(e.to);
        sf::Vector2f a = transform.toScreen(p1.x, p1.y);
        sf::Vector2f b = transform.toScreen(p2.x, p2.y);

        sf::Vector2f dir = b - a;
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (length < 1.f)
            continue;
        sf::Vector2f unit = {dir.x / length, dir.y / length};
        sf::Vector2f perp = {-unit.y, unit.x};

        float offset = ROAD_WIDTH / 2.f + 26.f;

        bool startWithTree = ((e.from * 31 + e.to * 17) % 2) == 0;

        for (float t : {0.33f, 0.67f})
        {
            sf::Vector2f pointOnRoad = a + unit * (length * t);
            sf::Vector2f sideA = pointOnRoad + perp * offset;
            sf::Vector2f sideB = pointOnRoad - perp * offset;

            bool tThis = startWithTree ? (t < 0.5f) : (t >= 0.5f);
            decorations_.push_back({sideA, tThis});
            decorations_.push_back({sideB, !tThis});
        }
    }
}

void MapRenderer::drawDecorations(sf::RenderWindow &window)
{
    for (const auto &d : decorations_)
    {
        if (d.isTree)
        {
            drawIconOrFallback(window, d.pos, TREE_SIZE, treeTexture_, treeTextureLoaded_,
                               sf::Color(50, 120, 60), true);
        }
        else
        {
            drawIconOrFallback(window, d.pos, HOUSE_SIZE, houseTexture_, houseTextureLoaded_,
                               sf::Color(150, 110, 80), false);
        }
    }
}

void MapRenderer::drawMap(sf::RenderWindow &window, const Graph &graph, const MapTransform &transform)
{
    for (const auto &e : graph.getAllEdgesForRender())
    {
        NodePos p1 = graph.getPosition(e.from);
        NodePos p2 = graph.getPosition(e.to);
        sf::Vector2f a = transform.toScreen(p1.x, p1.y);
        sf::Vector2f b = transform.toScreen(p2.x, p2.y);

        sf::Vector2f dir = b - a;
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        float angleDeg = std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;

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

        // Blocked road: broken tree trunk laid PERPENDICULAR across the road
        // (icon's own default orientation is assumed horizontal, so we add
        // the road's angle + 90 degrees to lay it crosswise).
        if (e.blocked)
        {
            sf::Vector2f mid = a + dir * 0.5f;
            drawIconOrFallback(window, mid, BROKEN_TREE_SIZE, brokenTreeTexture_,
                               brokenTreeTextureLoaded_, sf::Color(110, 70, 40), false,
                               angleDeg + 90.f);
        }
    }

    for (int id = 1; id <= 2000; ++id)
    {
        if (!graph.hasNode(id))
            continue;
        NodePos p = graph.getPosition(id);
        sf::Vector2f pos = transform.toScreen(p.x, p.y);

        sf::CircleShape circle(NODE_RADIUS);
        circle.setFillColor(sf::Color(235, 235, 240));
        circle.setOutlineColor(sf::Color(50, 55, 65));
        circle.setOutlineThickness(2.f);
        circle.setOrigin({NODE_RADIUS, NODE_RADIUS});
        circle.setPosition(pos);
        window.draw(circle);

        if (fontLoaded_)
        {
            sf::Text label(font_);
            label.setString(std::to_string(id));
            label.setCharacterSize(14);
            label.setFillColor(sf::Color(235, 235, 240));
            label.setPosition({pos.x - 4.f, pos.y - 26.f});
            window.draw(label);
        }
    }
    // ---------------- Fountain ----------------
if (fountainTextureLoaded_)
{
    sf::Vector2f pos = transform.toScreen(5, 5);

    sf::Sprite fountain(fountainTexture_);

    sf::Vector2u texSize = fountainTexture_.getSize();

    fountain.setOrigin({
        texSize.x / 2.f,
        texSize.y / 2.f
    });

    float desiredSize = 90.f;      // <-- Change this to resize

    float scale =
        desiredSize /
        static_cast<float>(std::max(texSize.x, texSize.y));

    fountain.setScale({scale, scale});

    fountain.setPosition(pos);

    window.draw(fountain);
}
}

void MapRenderer::drawHospitals(sf::RenderWindow &window, const std::vector<Hospital> &hospitals,
                                const Graph &graph, const MapTransform &transform)
{
    const float HOSPITAL_OFFSET_Y = -70.f; // pulls the icon above the node

    for (const auto &h : hospitals)
    {
        if (!graph.hasNode(h.nodeId))
            continue;
        NodePos p = graph.getPosition(h.nodeId);
        sf::Vector2f nodePos = transform.toScreen(p.x, p.y);
        sf::Vector2f pos = {nodePos.x, nodePos.y + HOSPITAL_OFFSET_Y};

        // thin connector line so it's still clear which node this hospital belongs to
        sf::Vertex line[] = {
            sf::Vertex{nodePos, sf::Color(60, 130, 220, 140)},
            sf::Vertex{pos, sf::Color(60, 130, 220, 140)}};
        window.draw(line, 2, sf::PrimitiveType::Lines);

        drawIconOrFallback(window, pos, HOSPITAL_SIZE, hospitalTexture_, hospitalTextureLoaded_,
                           sf::Color(60, 130, 220), false);

        if (fontLoaded_)
        {
            sf::Text label(font_);
            label.setString("H" + std::to_string(h.id));
            label.setCharacterSize(14);
            label.setFillColor(sf::Color::White);
            label.setPosition({pos.x - 10.f, pos.y - HOSPITAL_SIZE / 2.f - 20.f});
            window.draw(label);
        }
    }
}

void MapRenderer::drawAmbulanceAt(sf::RenderWindow &window, sf::Vector2f screenPos, int ambulanceId,
                                  float rotationDegrees)
{
    drawIconOrFallback(window, screenPos, AMBULANCE_SIZE, ambulanceTexture_, ambulanceTextureLoaded_,
                       sf::Color(220, 60, 60), true, rotationDegrees);

    if (fontLoaded_)
    {
        sf::Text label(font_);
        label.setString("A" + std::to_string(ambulanceId));
        label.setCharacterSize(12);
        label.setFillColor(sf::Color::White);
        label.setPosition({screenPos.x - 9.f, screenPos.y + AMBULANCE_SIZE / 2.f + 2.f});
        window.draw(label);
    }
}

sf::Vector2f MapRenderer::interpolateAlongPath(const std::vector<int> &pathNodes, float progress,
                                               const Graph &graph, const MapTransform &transform)
{
    if (pathNodes.empty())
        return {0.f, 0.f};
    if (pathNodes.size() == 1)
    {
        NodePos p = graph.getPosition(pathNodes[0]);
        return transform.toScreen(p.x, p.y);
    }

    progress = std::max(0.f, std::min(1.f, progress));

    int segmentCount = static_cast<int>(pathNodes.size()) - 1;
    float scaledProgress = progress * segmentCount;
    int segmentIndex = std::min(segmentCount - 1, static_cast<int>(scaledProgress));
    float segmentT = scaledProgress - segmentIndex;

    NodePos p1 = graph.getPosition(pathNodes[segmentIndex]);
    NodePos p2 = graph.getPosition(pathNodes[segmentIndex + 1]);
    sf::Vector2f a = transform.toScreen(p1.x, p1.y);
    sf::Vector2f b = transform.toScreen(p2.x, p2.y);

    return {
        a.x + (b.x - a.x) * segmentT,
        a.y + (b.y - a.y) * segmentT};
}

float MapRenderer::getHeadingAlongPath(const std::vector<int> &pathNodes, float progress,
                                       const Graph &graph, const MapTransform &transform)
{
    if (pathNodes.size() < 2)
        return 0.f;

    progress = std::max(0.f, std::min(1.f, progress));

    int segmentCount = static_cast<int>(pathNodes.size()) - 1;
    float scaledProgress = progress * segmentCount;
    int segmentIndex = std::min(segmentCount - 1, static_cast<int>(scaledProgress));

    NodePos p1 = graph.getPosition(pathNodes[segmentIndex]);
    NodePos p2 = graph.getPosition(pathNodes[segmentIndex + 1]);
    sf::Vector2f a = transform.toScreen(p1.x, p1.y);
    sf::Vector2f b = transform.toScreen(p2.x, p2.y);

    sf::Vector2f dir = b - a;
    return std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;
}