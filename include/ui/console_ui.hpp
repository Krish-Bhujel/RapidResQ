#pragma once
#include "sim/simulation.hpp"

class ConsoleUI {
public:
    // Prints the full event log in a readable format.
    static void printEventLog(const Simulation& sim);

    // Prints a final statistics summary based on the event log.
    static void printSummary(const Simulation& sim);
};