#include "ColorMap.hpp"

ColorMap::ColorMap(std::string name, std::array<glm::vec3, 7> coeffs) :
    name(name), coeffs(coeffs)
{

}

glm::vec3 ColorMap::get_color(float t) {
    return coeffs[0]+t*(coeffs[1]+t*(coeffs[2]+t*(coeffs[3]+t*(coeffs[4]+t*(coeffs[5]+t*coeffs[6])))));
}

void ColorMap::set_uniforms(Shader& shader) {
    for (int i = 0; i < coeffs.size(); i++)
        shader.set_vec3(("c" + std::to_string(i)).c_str(), coeffs[i]);
}