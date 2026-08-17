#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <optional>
#include <limits>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include "graph/graph.hpp"
#include "graph/maploader.hpp"
#include "sim/entityloader.hpp"
#include "algo/dijkstra.hpp"
#include "sim/simulation.hpp"
#include "ui/map_renderer.hpp"
#include "ui/console_ui.hpp"
#include "ui/button.hpp"
#include "ui/decoration.hpp"

enum class AppState { Ready, Running, Paused, Finished };

std::string severityName(IncidentSeverity s) {
    if (s == IncidentSeverity::Low) return "Low";
    if (s == IncidentSeverity::Medium) return "Medium";
    return "High";
}

float distanceToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) {
    sf::Vector2f ab = b - a;
    float lengthSq = ab.x * ab.x + ab.y * ab.y;
    float t = 0.f;
    if (lengthSq > 0.0001f) {
        t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSq;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
    }
    sf::Vector2f proj(a.x + ab.x * t, a.y + ab.y * t);
    sf::Vector2f diff(p.x - proj.x, p.y - proj.y);
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

float distanceToPoint(sf::Vector2f a, sf::Vector2f b) {
    sf::Vector2f d = a - b;
    return std::sqrt(d.x * d.x + d.y * d.y);
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    float MAP_WIDTH = 1050.f;
    float MAP_HEIGHT = 1050.f;
    float SIDEBAR_WIDTH = 340.f;
    float TOTAL_WIDTH = MAP_WIDTH + SIDEBAR_WIDTH;
    float NODE_CLICK_RADIUS = 16.f;

    Graph city;
    std::vector<Hospital> hospitals;
    std::vector<Ambulance> ambulances;

    // Loads (or reloads) everything from the data files.
    auto loadAll = [&]() -> bool {
        city.clear();
        hospitals.clear();
        ambulances.clear();
        if (!MapLoader::load(city, "../data/intersections.txt", "../data/roads.txt")) return false;
        if (!EntityLoader::loadHospitals("../data/hospitals.txt", hospitals)) return false;
        if (!EntityLoader::loadAmbulances("../data/ambulances.txt", ambulances)) return false;
        return true;
    };

    if (!loadAll()) {
        std::cerr << "Fatal: could not load initial data." << std::endl;
        return 1;
    }

    Dijkstra pathfinder;
    Simulation sim(city, pathfinder, ambulances, hospitals);
    if (!sim.loadScenario("../data/scenarios.txt")) {
        std::cerr << "Fatal: could not load scenario." << std::endl;
        return 1;
    }

    sf::RenderWindow window(sf::VideoMode({static_cast<unsigned int>(TOTAL_WIDTH),
                                            static_cast<unsigned int>(MAP_HEIGHT)}),
                             "RapidResQ - Live Simulation");

    MapTransform transform(city, MAP_WIDTH, MAP_HEIGHT, 140.f);
    MapRenderer renderer;
    renderer.loadFont("../assets/Arial.ttf");
    renderer.loadAmbulanceIcon("../assets/ambulance.png");
    renderer.loadBrokenTreeIcon("../assets/broken_tree.png");

    renderer.loadDecorationIcon("tree", "../assets/tree.png", 30.f);
    renderer.loadDecorationIcon("house", "../assets/house.png", 34.f);
    renderer.loadDecorationIcon("building", "../assets/building.png", 60.f);
    renderer.loadDecorationIcon("fountain", "../assets/fountain.png", 90.f);
    renderer.loadDecorationIcon("hospital", "../assets/hospital.png", 56.f);

    renderer.loadDecorationIcon("fountain1", "../assets/fountain_1.png", 90.f);
    renderer.loadDecorationIcon("Palm_trees", "../assets/Palm_trees.png", 20.f);
    renderer.loadDecorationIcon("cherry", "../assets/Cherry_blossom.png", 70.f);
    renderer.loadDecorationIcon("building", "../assets/building.png", 70.f);
    renderer.loadDecorationIcon("Hotel", "../assets/Hotel.png", 70.f);
    renderer.loadDecorationIcon("shop", "../assets/shop.png", 70.f);
    renderer.loadDecorationIcon("small_shop", "../assets/small_shop.png", 70.f);
    renderer.loadDecorationIcon("school", "../assets/school.png", 70.f);
    renderer.loadDecorationIcon("museum", "../assets/museum.png", 70.f);
    renderer.loadDecorationIcon("factory", "../assets/factory.png", 70.f);  
    renderer.loadDecorationIcon("lake", "../assets/lake.png", 90.f);
    renderer.loadDecorationIcon("hospital1", "../assets/hospital_1.png", 70.f);

    std::vector<DecorationEntry> decorations;
    DecorationLoader::load("../data/decorations.txt", decorations);
    renderer.setDecorations(decorations);

    sf::Font uiFont;
    bool uiFontLoaded = uiFont.openFromFile("../assets/Arial.ttf");

    std::vector<sf::Color> ambulanceColors;
    ambulanceColors.push_back(sf::Color(60, 200, 120, 140));
    ambulanceColors.push_back(sf::Color(80, 160, 255, 140));
    ambulanceColors.push_back(sf::Color(255, 170, 60, 140));
    ambulanceColors.push_back(sf::Color(200, 100, 255, 140));
    ambulanceColors.push_back(sf::Color(255, 120, 150, 140));
    ambulanceColors.push_back(sf::Color(255, 220, 80, 140));

    std::unordered_map<int, sf::Color> colorByAmbulanceId;
    for (int i = 0; i < ambulances.size(); i++) {
        colorByAmbulanceId[ambulances[i].id] = ambulanceColors[i % ambulanceColors.size()];
    }

    Button toggleButton({MAP_WIDTH + 20.f, 60.f}, {300.f, 46.f}, "Start");
    Button resetButton({MAP_WIDTH + 20.f, 116.f}, {300.f, 46.f}, "Reset");
    Button chaosButton({MAP_WIDTH + 20.f, 172.f}, {300.f, 46.f}, "Trigger Chaos Event");

    AppState appState = AppState::Ready;

    float TICK_INTERVAL_SECONDS = 1.0f;
    sf::Clock tickClock;
    sf::Clock pulseClock;
    int lastPrintedEvent = 0;

    while (window.isOpen()) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        toggleButton.updateHover(mousePos);
        resetButton.updateHover(mousePos);
        chaosButton.updateHover(mousePos);

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            const sf::Event::MouseButtonPressed* mousePressed = event->getIf<sf::Event::MouseButtonPressed>();
            if (mousePressed != nullptr) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    sf::Vector2f clickPos(static_cast<float>(mousePressed->position.x),
                                           static_cast<float>(mousePressed->position.y));

                    if (toggleButton.contains(clickPos)) {
                        if (appState == AppState::Running) {
                            appState = AppState::Paused;
                        } else {
                            appState = AppState::Running;
                            tickClock.restart();
                        }
                    }

                    if (resetButton.contains(clickPos)) {
                        loadAll();
                        sim.reset();
                        appState = AppState::Ready;
                        lastPrintedEvent = 0;
                        tickClock.restart();
                        std::cout << "--- Simulation reset ---" << std::endl;
                    }

                    if (chaosButton.contains(clickPos)) {
                        sim.triggerChaosEvent();
                        if (appState != AppState::Running) {
                            appState = AppState::Running;
                            tickClock.restart();
                        }
                    }

                    if (clickPos.x < MAP_WIDTH) {
                        int hitNode = -1;
                        std::vector<int> ids = city.getAllNodeIds();
                        for (int i = 0; i < ids.size(); i++) {
                            NodePos p = city.getPosition(ids[i]);
                            sf::Vector2f nodeScreen = transform.toScreen(p.x, p.y);
                            if (distanceToPoint(clickPos, nodeScreen) <= NODE_CLICK_RADIUS) {
                                hitNode = ids[i];
                                break;
                            }
                        }

                        if (hitNode != -1) {
                            IncidentSeverity sev = static_cast<IncidentSeverity>(std::rand() % 3);
                            sim.triggerManualIncident(hitNode, sev);
                        } else {
                            float bestDist = std::numeric_limits<float>::max();
                            int bestFrom = -1, bestTo = -1;

                            for (int i = 0; i < ids.size(); i++) {
                                int from = ids[i];
                                std::vector<Edge> neighbours = city.getAllNeighbours(from);
                                for (int j = 0; j < neighbours.size(); j++) {
                                    int to = neighbours[j].to;
                                    if (from >= to) continue;

                                    NodePos p1 = city.getPosition(from);
                                    NodePos p2 = city.getPosition(to);
                                    sf::Vector2f a = transform.toScreen(p1.x, p1.y);
                                    sf::Vector2f b = transform.toScreen(p2.x, p2.y);

                                    float dist = distanceToSegment(clickPos, a, b);
                                    if (dist < bestDist) {
                                        bestDist = dist;
                                        bestFrom = from;
                                        bestTo = to;
                                    }
                                }
                            }

                            float CLICK_THRESHOLD = MapRenderer::ROAD_WIDTH / 2.f + 14.f;
                            if (bestFrom != -1 && bestDist <= CLICK_THRESHOLD) {
                                if (city.isEdgeBlocked(bestFrom, bestTo)) {
                                    city.unblockEdge(bestFrom, bestTo);
                                    std::cout << "Road unblocked: " << bestFrom << " <-> " << bestTo << std::endl;
                                } else {
                                    city.blockEdge(bestFrom, bestTo);
                                    std::cout << "Road blocked: " << bestFrom << " <-> " << bestTo << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (appState == AppState::Running) toggleButton.setLabel("Pause");
        else if (appState == AppState::Paused) toggleButton.setLabel("Resume");
        else toggleButton.setLabel("Start");

        float elapsedSinceLastTick = tickClock.getElapsedTime().asSeconds();

        if (appState == AppState::Running && !sim.isFinished()) {
            if (elapsedSinceLastTick >= TICK_INTERVAL_SECONDS) {
                sim.tick();
                tickClock.restart();
                elapsedSinceLastTick = 0.f;

                std::vector<Event> logVec = sim.getEventLog();
                for (int i = lastPrintedEvent; i < logVec.size(); i++) {
                    std::cout << "[Tick " << logVec[i].tick << "] " << logVec[i].message << std::endl;
                }
                lastPrintedEvent = logVec.size();
            }
        }

        if (appState == AppState::Running && sim.isFinished()) {
            appState = AppState::Finished;
        }

        float subTickFraction = elapsedSinceLastTick / TICK_INTERVAL_SECONDS;
        if (subTickFraction > 1.f) subTickFraction = 1.f;
        if (appState != AppState::Running) subTickFraction = 0.f;

        window.clear(sf::Color(144, 238, 144));

        renderer.drawDecorations(window, transform);
        renderer.drawMap(window, city, transform);

        std::vector<Simulation::AmbulanceRenderState> ambStates(ambulances.size());
        std::unordered_map<int, std::vector<int>> parkedByNode;

        for (int i = 0; i < ambulances.size(); i++) {
            ambStates[i] = sim.getAmbulanceRenderState(ambulances[i].id, subTickFraction);
            if (!ambStates[i].isMoving) {
                parkedByNode[ambulances[i].currentNodeId].push_back(i);
            }
        }

        for (int i = 0; i < ambulances.size(); i++) {
            if (!ambStates[i].isMoving) continue;
            sf::Color col = sf::Color(100, 200, 100, 140);
            if (colorByAmbulanceId.count(ambulances[i].id) > 0) {
                col = colorByAmbulanceId[ambulances[i].id];
            }
            renderer.drawAmbulanceRoute(window, ambStates[i].fullRemainingRoute, city, transform, col);
        }

        float pulsePhase = std::fmod(pulseClock.getElapsedTime().asSeconds(), 1.5f) / 1.5f;
        std::vector<Incident> activeIncidents = sim.getActiveIncidents();
        for (int i = 0; i < activeIncidents.size(); i++) {
            if (!city.hasNode(activeIncidents[i].nodeId)) continue;
            NodePos p = city.getPosition(activeIncidents[i].nodeId);
            sf::Vector2f pos = transform.toScreen(p.x, p.y);
            renderer.drawIncidentBeacon(window, pos, pulsePhase);
        }

        float PARK_SPACING = 44.f;
        float PARK_OFFSET_Y = 55.f;

        for (int i = 0; i < ambulances.size(); i++) {
            Ambulance amb = ambulances[i];
            Simulation::AmbulanceRenderState state = ambStates[i];
            sf::Vector2f pos;
            float heading = 0.f;

            if (state.isMoving && state.currentEdgeNodes.size() == 2) {
                pos = renderer.interpolateAlongPath(state.currentEdgeNodes, state.progress, city, transform);
                heading = renderer.getHeadingAlongPath(state.currentEdgeNodes, state.progress, city, transform);
            } else {
                NodePos p = city.getPosition(amb.currentNodeId);
                sf::Vector2f basePos = transform.toScreen(p.x, p.y);

                std::vector<int> group = parkedByNode[amb.currentNodeId];
                int slotIndex = 0;
                for (int g = 0; g < group.size(); g++) {
                    if (group[g] == i) { slotIndex = g; break; }
                }
                float groupCount = static_cast<float>(group.size());
                float xOffset = (static_cast<float>(slotIndex) - (groupCount - 1) / 2.f) * PARK_SPACING;

                pos = sf::Vector2f(basePos.x + xOffset, basePos.y + PARK_OFFSET_Y);
            }

            renderer.drawAmbulanceAt(window, pos, amb.id, heading + 180.f);

            if (state.isMoving && uiFontLoaded) {
                sf::Text etaText(uiFont);
                etaText.setString("ETA: " + std::to_string(state.ticksRemainingTotal));
                etaText.setCharacterSize(13);
                etaText.setFillColor(sf::Color(255, 230, 140));
                etaText.setPosition({pos.x - 20.f, pos.y - 50.f});
                window.draw(etaText);
            }
        }

        sf::RectangleShape sidebarBg({SIDEBAR_WIDTH, MAP_HEIGHT});
        sidebarBg.setPosition({MAP_WIDTH, 0.f});
        sidebarBg.setFillColor(sf::Color(15, 20, 30));
        window.draw(sidebarBg);

        if (uiFontLoaded) {
            sf::Text title(uiFont);
            title.setString("RapidResQ Control Panel");
            title.setCharacterSize(20);
            title.setFillColor(sf::Color::White);
            title.setPosition({MAP_WIDTH + 20.f, 14.f});
            window.draw(title);
        }

        toggleButton.draw(window, uiFont, uiFontLoaded);
        resetButton.draw(window, uiFont, uiFontLoaded);
        chaosButton.draw(window, uiFont, uiFontLoaded);

        float infoY = 236.f;

        auto drawLine = [&](const std::string& text, sf::Color color) {
            if (!uiFontLoaded) return;
            sf::Text t(uiFont);
            t.setString(text);
            t.setCharacterSize(14);
            t.setFillColor(color);
            t.setPosition({MAP_WIDTH + 20.f, infoY});
            window.draw(t);
            infoY += 22.f;
        };

        sf::Color normalColor(220, 220, 225);

        std::string stateStr = "Ready";
        if (appState == AppState::Running) stateStr = "Running";
        else if (appState == AppState::Paused) stateStr = "Paused";
        else if (appState == AppState::Finished) stateStr = "Finished";

        drawLine("Tick: " + std::to_string(sim.currentTick()), normalColor);
        drawLine("State: " + stateStr, normalColor);
        infoY += 6.f;
        drawLine("Click a node: new incident", sf::Color(160, 200, 255));
        drawLine("Click a road: block/unblock", sf::Color(160, 200, 255));
        infoY += 10.f;

        int idleCount = 0, enRouteCount = 0, onSceneCount = 0, returningCount = 0;
        for (int i = 0; i < ambulances.size(); i++) {
            if (ambulances[i].status == AmbulanceStatus::Idle) idleCount++;
            else if (ambulances[i].status == AmbulanceStatus::EnRouteToIncident) enRouteCount++;
            else if (ambulances[i].status == AmbulanceStatus::OnScene) onSceneCount++;
            else if (ambulances[i].status == AmbulanceStatus::ReturningToHospital) returningCount++;
        }
        drawLine("Ambulances:", normalColor);
        drawLine("  Idle: " + std::to_string(idleCount), normalColor);
        drawLine("  En route: " + std::to_string(enRouteCount), normalColor);
        drawLine("  On scene: " + std::to_string(onSceneCount), normalColor);
        drawLine("  Returning: " + std::to_string(returningCount), normalColor);
        infoY += 10.f;

        drawLine("Active Incidents:", sf::Color(255, 180, 120));
        if (activeIncidents.size() == 0) {
            drawLine("  None", normalColor);
        } else {
            for (int i = 0; i < activeIncidents.size(); i++) {
                std::string line = "  Node " + std::to_string(activeIncidents[i].nodeId) +
                                    " (" + severityName(activeIncidents[i].severity) + ")";
                drawLine(line, normalColor);
            }
        }
        infoY += 10.f;

        drawLine("Blocked Roads:", sf::Color(255, 140, 140));
        std::vector<int> ids = city.getAllNodeIds();
        bool anyBlocked = false;
        for (int i = 0; i < ids.size(); i++) {
            int from = ids[i];
            std::vector<Edge> neighbours = city.getNeighbours(from); // note: blocked edges excluded here
        }
        // Simple approach: check every pair using isEdgeBlocked directly.
        for (int i = 0; i < ids.size(); i++) {
            for (int j = 0; j < ids.size(); j++) {
                int from = ids[i];
                int to = ids[j];
                if (from >= to) continue;
                if (city.isEdgeBlocked(from, to)) {
                    drawLine("  " + std::to_string(from) + " <-> " + std::to_string(to), normalColor);
                    anyBlocked = true;
                }
            }
        }
        if (!anyBlocked) {
            drawLine("  None", normalColor);
        }

        window.display();
    }

    ConsoleUI::printSummary(sim);

    return 0;
}