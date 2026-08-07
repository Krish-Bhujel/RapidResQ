cmake_minimum_required(VERSION 3.10)

project(RapidResQ)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(CMAKE_PREFIX_PATH "C:/msys64/mingw64")
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(
    rapidresq
    main.cpp
    src/graph/graph.cpp
    src/graph/maploader.cpp
    src/algo/dijkstra.cpp
    src/sim/entityloader.cpp
    src/sim/dispatcher.cpp
    src/sim/scenarioloader.cpp
    src/sim/simulation.cpp
    src/ui/console_ui.cpp
    src/ui/map_renderer.cpp
    src/ui/button.cpp
    src/ui/decorationloader.cpp
)

target_link_libraries(rapidresq SFML::Graphics SFML::Window SFML::System)