#include "ui/console_ui.hpp"
#include <iostream>

void ConsoleUI::printSummary(const Simulation& sim) {
    int incidentsDetected = 0;
    int ambulancesAssigned = 0;
    int incidentsResolved = 0;

    std::vector<Event> log = sim.getEventLog();
    for (int i = 0; i < log.size(); i++) {
        std::string msg = log[i].message;
        if (msg.find("Incident detected") != std::string::npos) incidentsDetected++;
        if (msg.find("assigned. Route cost") != std::string::npos) ambulancesAssigned++;
        if (msg.find("resolved") != std::string::npos) incidentsResolved++;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "              Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Incidents detected : " << incidentsDetected << std::endl;
    std::cout << "  Ambulances assigned: " << ambulancesAssigned << std::endl;
    std::cout << "  Incidents resolved : " << incidentsResolved << std::endl;
    std::cout << "  Total ticks run    : " << sim.currentTick() << std::endl;
    std::cout << "========================================" << std::endl;
}