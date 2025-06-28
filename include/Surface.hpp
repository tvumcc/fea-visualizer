#pragma once
#include <glm/glm.hpp>
#include <triangle/triangle.h>

#include "PSLG.hpp"

#include <vector>
#include <array>
#include <memory>

class Surface {
public:
    std::vector<glm::vec3> vertices;
    std::vector<std::array<int, 3>> triangles;
    std::vector<bool> on_boundary;

    bool closed;
    bool planar;

    void init_PSLG(PSLG& pslg);
    void init_obj();

    void draw();
private:
    bool initialized;
};