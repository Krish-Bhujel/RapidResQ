#pragma once
#include <string>
#include <vector>

struct DecorationEntry {
    std::string type;
    double x;
    double y;
    float size = -1.f;
};

class DecorationLoader {
public:
    static bool load(const std::string& path, std::vector<DecorationEntry>& outDecorations);
};