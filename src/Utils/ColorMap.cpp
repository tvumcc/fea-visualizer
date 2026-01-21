#include "Utils/ColorMap.hpp"

ColorMap::ColorMap(std::string name, std::array<glm::vec3, 7> coeffs) :
    name(name), coeffs(coeffs)
{}

/**
 * Evaluate the color map at a given value.
 * 
 * @param t The value to pass to the color map. This value should ideally be in the range [0.0, 1.0].
 */
glm::vec3 ColorMap::get_color(float t) {
    return coeffs[0] + t * (coeffs[1] + t * (coeffs[2] + t * (coeffs[3] + t * (coeffs[4] + t * (coeffs[5] + t * coeffs[6])))));
}

/**
 * Send all the coefficients of this color map to a shader.
 * Each coefficient's variable name follows the form c<number> from [0, 6] for a total of 7 coefficients.
 * Make sure to have an evaluation function in the shader. See ColorMap::get_color for a reference.
 * 
 * @param shader The shader whose uniforms will be set.
 */
void ColorMap::set_uniforms(Shader& shader) {
    for (int i = 0; i < coeffs.size(); i++)
        shader.set_vec3(("c" + std::to_string(i)).c_str(), coeffs[i]);
}