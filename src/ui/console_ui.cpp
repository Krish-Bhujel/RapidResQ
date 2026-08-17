#include "ui/console_ui.hpp"
#include <iostream>
#include <iomanip>

void ConsoleUI::printEventLog(const Simulation& sim) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "        RapidResQ Simulation Log" << std::endl;
    std::cout << "========================================\n" << std::endl;

    int lastTick = -1;
    for (const auto& e : sim.getEventLog()) {
        if (e.tick != lastTick) {
            std::cout << "\n--- Tick " << e.tick << " ---" << std::endl;
            lastTick = e.tick;
        }
        std::cout << "  " << e.message << std::endl;
    }
    std::cout << std::endl;
}

void ConsoleUI::printSummary(const Simulation& sim) {
    int incidentsDetected = 0;
    int ambulancesAssigned = 0;
    int incidentsResolved = 0;

    for (const auto& e : sim.getEventLog()) {
        if (e.message.find("Incident detected") != std::string::npos) incidentsDetected++;
        if (e.message.find("assigned. Route cost") != std::string::npos) ambulancesAssigned++;
        if (e.message.find("resolved") != std::string::npos) incidentsResolved++;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "              Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Incidents detected : " << incidentsDetected << std::endl;
    std::cout << "  Ambulances assigned: " << ambulancesAssigned << std::endl;
    std::cout << "  Incidents resolved : " << incidentsResolved << std::endl;
    std::cout << "  Total ticks run    : " << sim.currentTick() << std::endl;
    std::cout << "========================================\n" << std::endl;
}