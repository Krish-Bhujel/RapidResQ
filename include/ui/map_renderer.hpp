#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "graph/graph.hpp"
#include "sim/hospital.hpp"
#include "sim/ambulance.hpp"

class MapTransform {
public:
    MapTransform(const Graph& graph, float screenWidth, float screenHeight, float padding);
    sf::Vector2f toScreen(double x, double y) const;

private:
    double minX_, minY_, scale_;
    float padding_;
    float screenHeight_;
};

class MapRenderer {
public:
    MapRenderer();

    bool loadFont(const std::string& fontPath);
    bool loadAmbulanceIcon(const std::string& path);
    bool loadHospitalIcon(const std::string& path);
    bool loadTreeIcon(const std::string& path);
    bool loadHouseIcon(const std::string& path);
    bool loadBrokenTreeIcon(const std::string& path);

    void generateDecorations(const Graph& graph, const MapTransform& transform);

    void drawDecorations(sf::RenderWindow& window);
    void drawMap(sf::RenderWindow& window, const Graph& graph, const MapTransform& transform);
    void drawHospitals(sf::RenderWindow& window, const std::vector<Hospital>& hospitals,
                        const Graph& graph, const MapTransform& transform);

    // rotationDegrees: direction the ambulance should visually face.
    // 0 degrees = default icon orientation (assumed facing "east"/right).
    void drawAmbulanceAt(sf::RenderWindow& window, sf::Vector2f screenPos, int ambulanceId,
                          float rotationDegrees);

    sf::Vector2f interpolateAlongPath(const std::vector<int>& pathNodes, float progress,
                                        const Graph& graph, const MapTransform& transform);

    // Returns the heading (in degrees, atan2 convention) of the road segment
    // the ambulance is currently traveling along, for use with drawAmbulanceAt.
    float getHeadingAlongPath(const std::vector<int>& pathNodes, float progress,
                               const Graph& graph, const MapTransform& transform);

    static constexpr float ROAD_WIDTH = 40.f;
    static constexpr float AMBULANCE_SIZE = 48.f;
    static constexpr float HOSPITAL_SIZE = 56.f;
    static constexpr float NODE_RADIUS = 6.f;
    static constexpr float TREE_SIZE = 30.f;
    static constexpr float HOUSE_SIZE = 34.f;
    static constexpr float BROKEN_TREE_SIZE = 48.f;

private:
    struct Decoration {
        sf::Vector2f pos;
        bool isTree;
    };

    sf::Font font_;
    bool fontLoaded_ = false;

    sf::Texture ambulanceTexture_;
    bool ambulanceTextureLoaded_ = false;
    sf::Texture hospitalTexture_;
    bool hospitalTextureLoaded_ = false;
    sf::Texture treeTexture_;
    bool treeTextureLoaded_ = false;
    sf::Texture houseTexture_;
    bool houseTextureLoaded_ = false;
    sf::Texture brokenTreeTexture_;
    bool brokenTreeTextureLoaded_ = false;

    std::vector<Decoration> decorations_;

    void drawIconOrFallback(sf::RenderWindow& window, sf::Vector2f pos, float targetSize,
                             sf::Texture& texture, bool loaded, sf::Color fallbackColor,
                             bool fallbackIsCircle, float rotationDegrees = 0.f);
};