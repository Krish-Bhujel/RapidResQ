#pragma once
#include <vector>

struct Hospital {
    int id;
    int nodeId;        // location on the graph
    int capacity;       // max ambulances it can house
    std::vector<int> ambulanceIds; // ids of ambulances based here
};