#pragma once
#include <glm/glm.hpp>

#include "Shader.hpp"

#include <array>
#include <string>

class ColorMap {
public:
    std::string name;
    std::array<glm::vec3, 7> coeffs;

    ColorMap(std::string name, std::array<glm::vec3, 7> coeffs);

    glm::vec3 get_color(float t);
    void set_uniforms(Shader& shader);
};