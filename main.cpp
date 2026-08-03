#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <optional>
#include <SFML/Graphics.hpp>
#include "graph/graph.hpp"
#include "graph/maploader.hpp"
#include "sim/entityloader.hpp"
#include "algo/dijkstra.hpp"
#include "sim/simulation.hpp"
#include "ui/map_renderer.hpp"
#include "ui/console_ui.hpp"
#include "ui/button.hpp"
#include <unordered_map>

enum class AppState { Ready, Running, Paused, Finished };

static std::string severityName(IncidentSeverity s) {
    switch (s) {
        case IncidentSeverity::Low: return "Low";
        case IncidentSeverity::Medium: return "Medium";
        case IncidentSeverity::High: return "High";
    }
    return "?";
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    const float MAP_WIDTH = 1150.f;
    const float MAP_HEIGHT = 850.f;
    const float SIDEBAR_WIDTH = 340.f;
    const float TOTAL_WIDTH = MAP_WIDTH + SIDEBAR_WIDTH;

    Graph city;
    std::vector<Hospital> hospitals;
    std::vector<Ambulance> ambulances;

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
    renderer.loadHospitalIcon("../assets/hospital.png");
    renderer.loadTreeIcon("../assets/tree.png");
    renderer.loadHouseIcon("../assets/house.png");
    renderer.loadBrokenTreeIcon("../assets/broken_tree.png");
    renderer.generateDecorations(city, transform);

    sf::Font uiFont;
    bool uiFontLoaded = uiFont.openFromFile("../assets/Arial.ttf");

    Button toggleButton({MAP_WIDTH + 20.f, 60.f}, {300.f, 46.f}, "Start");
    Button resetButton({MAP_WIDTH + 20.f, 116.f}, {300.f, 46.f}, "Reset");
    Button randomIncidentButton({MAP_WIDTH + 20.f, 172.f}, {300.f, 46.f}, "Trigger Random Incident");

    AppState appState = AppState::Ready;

    const float TICK_INTERVAL_SECONDS = 1.0f;
    sf::Clock tickClock;
    size_t lastPrintedEvent = 0;

    while (window.isOpen()) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        toggleButton.updateHover(mousePos);
        resetButton.updateHover(mousePos);
        randomIncidentButton.updateHover(mousePos);

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
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

                    if (randomIncidentButton.contains(clickPos)) {
                        std::vector<int> ids = city.getAllNodeIds();
                        if (!ids.empty()) {
                            int node = ids[std::rand() % ids.size()];
                            IncidentSeverity sev = static_cast<IncidentSeverity>(std::rand() % 3);
                            sim.triggerManualIncident(node, sev);
                        }
                    }
                }
            }
        }

        if (appState == AppState::Running) {
            toggleButton.setLabel("Pause");
        } else if (appState == AppState::Paused) {
            toggleButton.setLabel("Resume");
        } else {
            toggleButton.setLabel("Start");
        }

        float elapsedSinceLastTick = tickClock.getElapsedTime().asSeconds();

        if (appState == AppState::Running && !sim.isFinished()) {
            if (elapsedSinceLastTick >= TICK_INTERVAL_SECONDS) {
                sim.tick();
                tickClock.restart();
                elapsedSinceLastTick = 0.f;

                const auto& log = sim.getEventLog();
                for (size_t i = lastPrintedEvent; i < log.size(); ++i) {
                    std::cout << "[Tick " << log[i].tick << "] " << log[i].message << std::endl;
                }
                lastPrintedEvent = log.size();
            }
        }

        if (appState == AppState::Running && sim.isFinished()) {
            appState = AppState::Finished;
        }

        float subTickFraction = elapsedSinceLastTick / TICK_INTERVAL_SECONDS;
        if (subTickFraction > 1.f) subTickFraction = 1.f;
        if (appState != AppState::Running) subTickFraction = 0.f;

        window.clear(sf::Color(20, 35, 30));

        renderer.drawDecorations(window);
        renderer.drawMap(window, city, transform);
        renderer.drawHospitals(window, hospitals, city, transform);

        // First pass: compute each ambulance's render state, and group any
// non-moving (parked) ambulances by the node they're sitting at.
std::vector<Simulation::AmbulanceRenderState> ambStates(ambulances.size());
std::unordered_map<int, std::vector<size_t>> parkedByNode;

for (size_t i = 0; i < ambulances.size(); ++i) {
    ambStates[i] = sim.getAmbulanceRenderState(ambulances[i].id, subTickFraction);
    if (!ambStates[i].isMoving) {
        parkedByNode[ambulances[i].currentNodeId].push_back(i);
    }
}

const float PARK_SPACING = 44.f;
const float PARK_OFFSET_Y = 55.f; // below the node, opposite side from the hospital

for (size_t i = 0; i < ambulances.size(); ++i) {
    const auto& amb = ambulances[i];
    const auto& state = ambStates[i];
    sf::Vector2f pos;
    float heading = 0.f;

    if (state.isMoving && !state.routeNodes.empty()) {
        pos = renderer.interpolateAlongPath(state.routeNodes, state.progress, city, transform);
        heading = renderer.getHeadingAlongPath(state.routeNodes, state.progress, city, transform);
    } else {
        NodePos p = city.getPosition(amb.currentNodeId);
        sf::Vector2f basePos = transform.toScreen(p.x, p.y);

        auto& group = parkedByNode[amb.currentNodeId];
        size_t slotIndex = 0;
        for (size_t g = 0; g < group.size(); ++g) {
            if (group[g] == i) { slotIndex = g; break; }
        }
        float groupCount = static_cast<float>(group.size());
        float xOffset = (static_cast<float>(slotIndex) - (groupCount - 1) / 2.f) * PARK_SPACING;

        pos = { basePos.x + xOffset, basePos.y + PARK_OFFSET_Y };
    }

    renderer.drawAmbulanceAt(window, pos, amb.id, heading + 180.f);

    if (state.isMoving && state.headingToIncident && uiFontLoaded) {
        sf::Text etaText(uiFont);
        etaText.setString("ETA: " + std::to_string(state.ticksRemaining));
        etaText.setCharacterSize(13);
        etaText.setFillColor(sf::Color(255, 230, 140));
        etaText.setPosition({pos.x - 20.f, pos.y - 50.f});
        window.draw(etaText);
    }
}

        // ---------------- Sidebar ----------------
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
        randomIncidentButton.draw(window, uiFont, uiFontLoaded);

        float infoY = 236.f;
        auto drawInfoLine = [&](const std::string& text, sf::Color color = sf::Color(220, 220, 225)) {
            if (!uiFontLoaded) return;
            sf::Text t(uiFont);
            t.setString(text);
            t.setCharacterSize(14);
            t.setFillColor(color);
            t.setPosition({MAP_WIDTH + 20.f, infoY});
            window.draw(t);
            infoY += 22.f;
        };

        std::string stateStr = "Ready";
        if (appState == AppState::Running) stateStr = "Running";
        else if (appState == AppState::Paused) stateStr = "Paused";
        else if (appState == AppState::Finished) stateStr = "Finished";

        drawInfoLine("Tick: " + std::to_string(sim.currentTick()));
        drawInfoLine("State: " + stateStr);
        infoY += 10.f;

        int idleCount = 0, enRouteCount = 0, onSceneCount = 0, returningCount = 0;
        for (const auto& a : ambulances) {
            switch (a.status) {
                case AmbulanceStatus::Idle: idleCount++; break;
                case AmbulanceStatus::EnRouteToIncident: enRouteCount++; break;
                case AmbulanceStatus::OnScene: onSceneCount++; break;
                case AmbulanceStatus::ReturningToHospital: returningCount++; break;
            }
        }
        drawInfoLine("Ambulances:");
        drawInfoLine("  Idle: " + std::to_string(idleCount));
        drawInfoLine("  En route: " + std::to_string(enRouteCount));
        drawInfoLine("  On scene: " + std::to_string(onSceneCount));
        drawInfoLine("  Returning: " + std::to_string(returningCount));
        infoY += 10.f;

        drawInfoLine("Active Incidents:", sf::Color(255, 180, 120));
        std::vector<Incident> activeIncidents = sim.getActiveIncidents();
        if (activeIncidents.empty()) {
            drawInfoLine("  None");
        } else {
            for (const auto& inc : activeIncidents) {
                std::ostringstream oss;
                oss << "  Node " << inc.nodeId << " (" << severityName(inc.severity) << ")";
                drawInfoLine(oss.str());
            }
        }
        infoY += 10.f;

        drawInfoLine("Blocked Roads:", sf::Color(255, 140, 140));
        std::vector<std::pair<int, int>> blocked = city.getBlockedEdges();
        if (blocked.empty()) {
            drawInfoLine("  None");
        } else {
            for (const auto& b : blocked) {
                drawInfoLine("  " + std::to_string(b.first) + " <-> " + std::to_string(b.second));
            }
        }

        window.display();
    }

    ConsoleUI::printSummary(sim);

    return 0;
}