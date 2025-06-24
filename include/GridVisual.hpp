#include <glm/glm.hpp>

#include "Mesh.hpp"

class GridVisual {
public:  
    glm::vec3 default_color = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 x_axis_color = glm::vec3(1.0f, 0.0, 0.0f);
    glm::vec3 y_axis_color = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 z_axis_color = glm::vec3(0.0f, 1.0f, 0.0f);
    float grid_spacing = 1.0f;
    int grid_lines_per_quadrant = 50;

    Mesh panning_locator_mesh;
    glm::vec3 panning_locator_color = glm::vec3(0.7f, 0.0f, 0.7f);

    GridVisual();
    void draw_grid(Shader& shader);
    void draw_panning_locator(Shader& shader, glm::vec3 position);
private:
    unsigned int vertex_buffer;
    unsigned int vertex_array;
    std::vector<glm::vec3> vertices;
};