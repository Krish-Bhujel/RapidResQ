#include "graph/maploader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

static bool isBlankOrComment(const  string& line) {
    for (char c : line) {
        if (c == '#') return true;
        if (!isspace(static_cast<unsigned char>(c))) return false;
    }
    return true; // all whitespace
}

bool MapLoader::load(Graph& graph, const  string& intersectionsPath, const  string& roadsPath) {
     ifstream nodesFile(intersectionsPath);
    if (!nodesFile.is_open()) {
         cerr << "MapLoader: could not open " << intersectionsPath <<  endl;
        return false;
    }

     string line;
    int lineNumber = 0;
    while ( getline(nodesFile, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

         istringstream iss(line);
        int id;
        double x, y;
        if (!(iss >> id >> x >> y)) {
             cerr << "MapLoader: malformed line " << lineNumber
                      << " in " << intersectionsPath << ": \"" << line << "\"" <<  endl;
            continue;
        }
        try {
            graph.addNode(id, x, y);
        } catch (const  exception& e) {
             cerr << "MapLoader: " << e.what() << " (line " << lineNumber << ")" <<  endl;
        }
    }

     ifstream edgesFile(roadsPath);
    if (!edgesFile.is_open()) {
         cerr << "MapLoader: could not open " << roadsPath <<  endl;
        return false;
    }

    lineNumber = 0;
    while ( getline(edgesFile, line)) {
        lineNumber++;
        if (isBlankOrComment(line)) continue;

         istringstream iss(line);
        int from, to;
        double weight;
        if (!(iss >> from >> to >> weight)) {
             cerr << "MapLoader: malformed line " << lineNumber
                      << " in " << roadsPath << ": \"" << line << "\"" <<  endl;
            continue;
        }
        try {
            graph.addEdge(from, to, weight);
        } catch (const  exception& e) {
             cerr << "MapLoader: " << e.what() << " (line " << lineNumber << ")" <<  endl;
        }
    }

    return true;
}