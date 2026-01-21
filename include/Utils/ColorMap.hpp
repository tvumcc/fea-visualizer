#pragma once
#include "Utils/Shader.hpp"

#include <array>
#include <string>

/**
 * A color map encoded by an order 6 polynomial.
 * All coefficients are taken from https://www.shadertoy.com/view/Nd3fR2.
 */
class ColorMap {
public:
    std::string name;
    std::array<glm::vec3, 7> coeffs;

    ColorMap(std::string name, std::array<glm::vec3, 7> coeffs);

    glm::vec3 get_color(float t);
    void set_uniforms(Shader& shader);
};