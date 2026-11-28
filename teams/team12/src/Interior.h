#pragma once

#include "Geometry.h"

namespace team12 {

struct InteriorSurface {
    Mesh mesh;
    glm::vec3 color;
    bool display = false;
};

struct StadiumInterior {
    std::vector<InteriorSurface> pitchBoards;
    std::vector<InteriorSurface> ribbons;
    std::vector<InteriorSurface> banners;
    std::vector<InteriorSurface> flags;
};

StadiumInterior makeStadiumInterior(const StadiumLayout& layout);
void drawStadiumInterior(const StadiumInterior& interior);

}  // namespace team12
