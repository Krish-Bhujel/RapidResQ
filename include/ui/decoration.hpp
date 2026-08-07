#pragma once
#include <string>
#include <vector>

struct DecorationEntry {
    std::string type;  // e.g. "tree", "house", "building", "small_house", "fountain" —
                        // must match a name registered via MapRenderer::loadDecorationIcon
    double x;
    double y;
    float size = -1.f; // -1 means "use this type's default size"
};

class DecorationLoader {
public:
    // File format, one decoration per line:
    //   type x y [size]
    // size is optional — omit it (or use 0) to fall back to the type's default size.
    static bool load(const std::string& path, std::vector<DecorationEntry>& outDecorations);
};