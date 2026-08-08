#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include "graph/graph.hpp"
#include "sim/hospital.hpp"
#include "sim/ambulance.hpp"
#include "ui/decoration.hpp"

class MapTransform
{
public:
    MapTransform(const Graph &graph, float screenWidth, float screenHeight, float padding);
    sf::Vector2f toScreen(double x, double y) const;

private:
    double minX_, minY_, scale_;
    float padding_;
    float screenWidth_;
    float screenHeight_;
};

class MapRenderer
{
public:
    MapRenderer();

    bool loadFont(const std::string &fontPath);
    bool loadAmbulanceIcon(const std::string &path);
    bool loadHospitalIcon(const std::string &path);
    bool loadBrokenTreeIcon(const std::string &path);

    void drawAmbulanceRoute(sf::RenderWindow &window, const std::vector<int> &routeNodes,
                            const Graph &graph, const MapTransform &transform, sf::Color color);

    void drawIncidentBeacon(sf::RenderWindow &window, sf::Vector2f nodeScreenPos, float pulsePhase01);

    // Registers a decoration type (e.g. "tree", "house", "building", "small_house", "fountain").
    // defaultSize is the pixel size used for any decoration of this type that doesn't
    // specify its own size in decorations.txt. Call this once per type, in main.cpp,
    // BEFORE calling setDecorations().
    //   >>> THIS defaultSize PARAMETER IS WHERE YOU ADJUST EACH TYPE'S SIZE <
    bool loadDecorationIcon(const std::string &type, const std::string &path, float defaultSize);

    // Replaces the full list of manually-placed decorations (output of DecorationLoader::load).
    void setDecorations(const std::vector<DecorationEntry> &decorations);

    void drawDecorations(sf::RenderWindow &window, const MapTransform &transform);
    void drawMap(sf::RenderWindow &window, const Graph &graph, const MapTransform &transform);
    void drawHospitals(sf::RenderWindow &window, const std::vector<Hospital> &hospitals,
                       const Graph &graph, const MapTransform &transform);

    void drawAmbulanceAt(sf::RenderWindow &window, sf::Vector2f screenPos, int ambulanceId,
                         float rotationDegrees);

    sf::Vector2f interpolateAlongPath(const std::vector<int> &pathNodes, float progress,
                                      const Graph &graph, const MapTransform &transform);

    float getHeadingAlongPath(const std::vector<int> &pathNodes, float progress,
                              const Graph &graph, const MapTransform &transform);

    static constexpr float ROAD_WIDTH = 26.f;
    static constexpr float AMBULANCE_SIZE = 48.f;
    static constexpr float HOSPITAL_SIZE = 56.f;
    static constexpr float NODE_RADIUS = 6.f;
    static constexpr float BROKEN_TREE_SIZE = 48.f;
    // Fallback size ONLY used if a decoration's type was never registered via loadDecorationIcon.
    static constexpr float DEFAULT_DECORATION_SIZE = 32.f;

private:
    sf::Font font_;
    bool fontLoaded_ = false;

    sf::Texture ambulanceTexture_;
    bool ambulanceTextureLoaded_ = false;
    sf::Texture hospitalTexture_;
    bool hospitalTextureLoaded_ = false;
    sf::Texture brokenTreeTexture_;
    bool brokenTreeTextureLoaded_ = false;

    // type name -> loaded texture, and type name -> its registered default size
    std::unordered_map<std::string, sf::Texture> decorationTextures_;
    std::unordered_map<std::string, float> decorationDefaultSizes_;

    std::vector<DecorationEntry> decorations_;

    void drawIconOrFallback(sf::RenderWindow &window, sf::Vector2f pos, float targetSize,
                            sf::Texture &texture, bool loaded, sf::Color fallbackColor,
                            bool fallbackIsCircle, float rotationDegrees = 0.f);
};