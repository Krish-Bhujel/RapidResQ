#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sstream>
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

enum class AppState
{
    Ready,
    Running,
    Paused,
    Finished
};

static std::string severityName(IncidentSeverity s)
{
    switch (s)
    {
    case IncidentSeverity::Low:
        return "Low";
    case IncidentSeverity::Medium:
        return "Medium";
    case IncidentSeverity::High:
        return "High";
    }
    return "?";
}

static float distanceToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b)
{
    sf::Vector2f ab = b - a;
    float lengthSq = ab.x * ab.x + ab.y * ab.y;
    float t = 0.f;
    if (lengthSq > 0.0001f)
    {
        t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSq;
        t = std::max(0.f, std::min(1.f, t));
    }
    sf::Vector2f proj = {a.x + ab.x * t, a.y + ab.y * t};
    sf::Vector2f diff = {p.x - proj.x, p.y - proj.y};
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    const float MAP_WIDTH = 1050.f;
    const float MAP_HEIGHT = 1050.f;
    const float SIDEBAR_WIDTH = 340.f;
    const float TOTAL_WIDTH = MAP_WIDTH + SIDEBAR_WIDTH;

    Graph city;
    std::vector<Hospital> hospitals;
    std::vector<Ambulance> ambulances;

    auto loadAll = [&]() -> bool
    {
        city.clear();
        hospitals.clear();
        ambulances.clear();
        if (!MapLoader::load(city, "../data/intersections.txt", "../data/roads.txt"))
            return false;
        if (!EntityLoader::loadHospitals("../data/hospitals.txt", hospitals))
            return false;
        if (!EntityLoader::loadAmbulances("../data/ambulances.txt", ambulances))
            return false;
        return true;
    };

    if (!loadAll())
    {
        std::cerr << "Fatal: could not load initial data." << std::endl;
        return 1;
    }

    Dijkstra pathfinder;
    Simulation sim(city, pathfinder, ambulances, hospitals);
    if (!sim.loadScenario("../data/scenarios.txt"))
    {
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
    renderer.loadBrokenTreeIcon("../assets/broken_tree.png");

    // ---------------------------------------------------------------
    // Register every decoration TYPE here: name, .png path, default size.
    //   >>> SIZE ADJUSTMENT HAPPENS HERE (3rd argument) <
    // Add one line per decoration type you want to use in decorations.txt.
    // The name (1st argument) must exactly match the "type" column you use
    // in data/decorations.txt.
    // ---------------------------------------------------------------
    renderer.loadDecorationIcon("tree", "../assets/tree.png", 30.f);
    renderer.loadDecorationIcon("house", "../assets/house.png", 34.f);
    renderer.loadDecorationIcon("small_house", "../assets/small_house.png", 24.f);
    renderer.loadDecorationIcon("building", "../assets/building.png", 60.f);
    renderer.loadDecorationIcon("fountain", "../assets/fountain.png", 90.f);

    // Load the actual placed decorations (positions) from the data file.
    std::vector<DecorationEntry> decorations;
    if (!DecorationLoader::load("../data/decorations.txt", decorations)) {
        std::cerr << "Warning: could not load decorations.txt (continuing with no decorations)." << std::endl;
    }
    renderer.setDecorations(decorations);

    sf::Font uiFont;
    bool uiFontLoaded = uiFont.openFromFile("../assets/Arial.ttf");

    Button toggleButton({MAP_WIDTH + 20.f, 60.f}, {300.f, 46.f}, "Start");
    Button resetButton({MAP_WIDTH + 20.f, 116.f}, {300.f, 46.f}, "Reset");
    Button randomIncidentButton({MAP_WIDTH + 20.f, 172.f}, {300.f, 46.f}, "Trigger Random Incident");

    AppState appState = AppState::Ready;

    const float TICK_INTERVAL_SECONDS = 1.0f;
    sf::Clock tickClock;
    size_t lastPrintedEvent = 0;

    while (window.isOpen())
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        toggleButton.updateHover(mousePos);
        resetButton.updateHover(mousePos);
        randomIncidentButton.updateHover(mousePos);

        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mousePressed->button == sf::Mouse::Button::Left)
                {
                    sf::Vector2f clickPos(static_cast<float>(mousePressed->position.x),
                                           static_cast<float>(mousePressed->position.y));

                    if (toggleButton.contains(clickPos))
                    {
                        if (appState == AppState::Running)
                        {
                            appState = AppState::Paused;
                        }
                        else
                        {
                            appState = AppState::Running;
                            tickClock.restart();
                        }
                    }

                    if (resetButton.contains(clickPos))
                    {
                        loadAll();
                        sim.reset();
                        appState = AppState::Ready;
                        lastPrintedEvent = 0;
                        tickClock.restart();
                        std::cout << "--- Simulation reset ---" << std::endl;
                    }

                    if (randomIncidentButton.contains(clickPos))
                    {
                        std::vector<int> ids = city.getAllNodeIds();
                        if (!ids.empty())
                        {
                            int node = ids[std::rand() % ids.size()];
                            IncidentSeverity sev = static_cast<IncidentSeverity>(std::rand() % 3);
                            sim.triggerManualIncident(node, sev);
                        }
                    }

                    if (clickPos.x < MAP_WIDTH)
                    {
                        float bestDist = std::numeric_limits<float>::max();
                        int bestFrom = -1, bestTo = -1;

                        for (const auto &e : city.getAllEdgesForRender())
                        {
                            NodePos p1 = city.getPosition(e.from);
                            NodePos p2 = city.getPosition(e.to);
                            sf::Vector2f a = transform.toScreen(p1.x, p1.y);
                            sf::Vector2f b = transform.toScreen(p2.x, p2.y);

                            float dist = distanceToSegment(clickPos, a, b);
                            if (dist < bestDist)
                            {
                                bestDist = dist;
                                bestFrom = e.from;
                                bestTo = e.to;
                            }
                        }

                        const float CLICK_THRESHOLD = MapRenderer::ROAD_WIDTH / 2.f + 14.f;
                        if (bestFrom != -1 && bestDist <= CLICK_THRESHOLD)
                        {
                            bool currentlyBlocked = false;
                            for (const auto &e : city.getAllEdgesForRender())
                            {
                                if ((e.from == bestFrom && e.to == bestTo) || (e.from == bestTo && e.to == bestFrom))
                                {
                                    currentlyBlocked = e.blocked;
                                    break;
                                }
                            }

                            if (currentlyBlocked)
                            {
                                city.unblockEdge(bestFrom, bestTo);
                                std::cout << "Road unblocked (manual click): " << bestFrom << " <-> " << bestTo << std::endl;
                            }
                            else
                            {
                                city.blockEdge(bestFrom, bestTo);
                                std::cout << "Road blocked (manual click): " << bestFrom << " <-> " << bestTo << std::endl;
                            }
                        }
                    }
                }
            }
        }

        if (appState == AppState::Running)
        {
            toggleButton.setLabel("Pause");
        }
        else if (appState == AppState::Paused)
        {
            toggleButton.setLabel("Resume");
        }
        else
        {
            toggleButton.setLabel("Start");
        }

        float elapsedSinceLastTick = tickClock.getElapsedTime().asSeconds();

        if (appState == AppState::Running && !sim.isFinished())
        {
            if (elapsedSinceLastTick >= TICK_INTERVAL_SECONDS)
            {
                sim.tick();
                tickClock.restart();
                elapsedSinceLastTick = 0.f;

                const auto &log = sim.getEventLog();
                for (size_t i = lastPrintedEvent; i < log.size(); ++i)
                {
                    std::cout << "[Tick " << log[i].tick << "] " << log[i].message << std::endl;
                }
                lastPrintedEvent = log.size();
            }
        }

        if (appState == AppState::Running && sim.isFinished())
        {
            appState = AppState::Finished;
        }

        float subTickFraction = elapsedSinceLastTick / TICK_INTERVAL_SECONDS;
        if (subTickFraction > 1.f)
            subTickFraction = 1.f;
        if (appState != AppState::Running)
            subTickFraction = 0.f;

        window.clear(sf::Color(144, 238, 144));

        renderer.drawDecorations(window, transform);
        renderer.drawMap(window, city, transform);
        renderer.drawHospitals(window, hospitals, city, transform);

        std::vector<Simulation::AmbulanceRenderState> ambStates(ambulances.size());
        std::unordered_map<int, std::vector<size_t>> parkedByNode;

        for (size_t i = 0; i < ambulances.size(); ++i)
        {
            ambStates[i] = sim.getAmbulanceRenderState(ambulances[i].id, subTickFraction);
            if (!ambStates[i].isMoving)
            {
                parkedByNode[ambulances[i].currentNodeId].push_back(i);
            }
        }

        const float PARK_SPACING = 44.f;
        const float PARK_OFFSET_Y = 55.f;

        for (size_t i = 0; i < ambulances.size(); ++i)
        {
            const auto &amb = ambulances[i];
            const auto &state = ambStates[i];
            sf::Vector2f pos;
            float heading = 0.f;

            if (state.isMoving && !state.routeNodes.empty())
            {
                pos = renderer.interpolateAlongPath(state.routeNodes, state.progress, city, transform);
                heading = renderer.getHeadingAlongPath(state.routeNodes, state.progress, city, transform);
            }
            else
            {
                NodePos p = city.getPosition(amb.currentNodeId);
                sf::Vector2f basePos = transform.toScreen(p.x, p.y);

                auto &group = parkedByNode[amb.currentNodeId];
                size_t slotIndex = 0;
                for (size_t g = 0; g < group.size(); ++g)
                {
                    if (group[g] == i)
                    {
                        slotIndex = g;
                        break;
                    }
                }
                float groupCount = static_cast<float>(group.size());
                float xOffset = (static_cast<float>(slotIndex) - (groupCount - 1) / 2.f) * PARK_SPACING;

                pos = {basePos.x + xOffset, basePos.y + PARK_OFFSET_Y};
            }

            renderer.drawAmbulanceAt(window, pos, amb.id, heading + 180.f);

            if (state.isMoving && state.headingToIncident && uiFontLoaded)
            {
                sf::Text etaText(uiFont);
                etaText.setString("ETA: " + std::to_string(state.ticksRemaining));
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

        if (uiFontLoaded)
        {
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
        auto drawInfoLine = [&](const std::string &text, sf::Color color = sf::Color(220, 220, 225))
        {
            if (!uiFontLoaded)
                return;
            sf::Text t(uiFont);
            t.setString(text);
            t.setCharacterSize(14);
            t.setFillColor(color);
            t.setPosition({MAP_WIDTH + 20.f, infoY});
            window.draw(t);
            infoY += 22.f;
        };

        std::string stateStr = "Ready";
        if (appState == AppState::Running)
            stateStr = "Running";
        else if (appState == AppState::Paused)
            stateStr = "Paused";
        else if (appState == AppState::Finished)
            stateStr = "Finished";

        drawInfoLine("Tick: " + std::to_string(sim.currentTick()));
        drawInfoLine("State: " + stateStr);
        infoY += 10.f;

        int idleCount = 0, enRouteCount = 0, onSceneCount = 0, returningCount = 0;
        for (const auto &a : ambulances)
        {
            switch (a.status)
            {
            case AmbulanceStatus::Idle:
                idleCount++;
                break;
            case AmbulanceStatus::EnRouteToIncident:
                enRouteCount++;
                break;
            case AmbulanceStatus::OnScene:
                onSceneCount++;
                break;
            case AmbulanceStatus::ReturningToHospital:
                returningCount++;
                break;
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
        if (activeIncidents.empty())
        {
            drawInfoLine("  None");
        }
        else
        {
            for (const auto &inc : activeIncidents)
            {
                std::ostringstream oss;
                oss << "  Node " << inc.nodeId << " (" << severityName(inc.severity) << ")";
                drawInfoLine(oss.str());
            }
        }
        infoY += 10.f;

        drawInfoLine("Blocked Roads:", sf::Color(255, 140, 140));
        std::vector<std::pair<int, int>> blocked = city.getBlockedEdges();
        if (blocked.empty())
        {
            drawInfoLine("  None");
        }
        else
        {
            for (const auto &b : blocked)
            {
                drawInfoLine("  " + std::to_string(b.first) + " <-> " + std::to_string(b.second));
            }
        }

        window.display();
    }

    ConsoleUI::printSummary(sim);

    return 0;
}