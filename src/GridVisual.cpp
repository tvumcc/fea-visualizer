#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "GridVisual.hpp"

#include <vector>

GridVisual::GridVisual() :
    panning_locator_mesh("assets/sphere.obj")
{
    for (int i = -grid_lines_per_quadrant; i <= grid_lines_per_quadrant; i++) {
        if (i != 0) {
            vertices.push_back(glm::vec3(i * grid_spacing, 0.0f, grid_lines_per_quadrant * grid_spacing));
            vertices.push_back(default_color);
            vertices.push_back(glm::vec3(i * grid_spacing, 0.0f, -grid_lines_per_quadrant * grid_spacing));
            vertices.push_back(default_color);

            vertices.push_back(glm::vec3(grid_lines_per_quadrant * grid_spacing, 0.0f, i * grid_spacing));
            vertices.push_back(default_color);
            vertices.push_back(glm::vec3(-grid_lines_per_quadrant * grid_spacing, 0.0f, i * grid_spacing));
            vertices.push_back(default_color);
        }
    }

    vertices.push_back(glm::vec3(grid_lines_per_quadrant * grid_spacing, 0.0f, 0.0f));
    vertices.push_back(x_axis_color);
    vertices.push_back(glm::vec3(-grid_lines_per_quadrant * grid_spacing, 0.0f, 0.0f));
    vertices.push_back(x_axis_color);

    vertices.push_back(glm::vec3(0.0f, grid_lines_per_quadrant * grid_spacing, 0.0f));
    vertices.push_back(y_axis_color);
    vertices.push_back(glm::vec3(0.0f, -grid_lines_per_quadrant * grid_spacing, 0.0f));
    vertices.push_back(y_axis_color);

    vertices.push_back(glm::vec3(0.0f, 0.0f, grid_lines_per_quadrant * grid_spacing));
    vertices.push_back(z_axis_color);
    vertices.push_back(glm::vec3(0.0f, 0.0f, -grid_lines_per_quadrant * grid_spacing));
    vertices.push_back(z_axis_color);

    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &vertex_buffer); 
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
}

void GridVisual::draw_grid(Shader& shader) {
    shader.bind();
    glBindVertexArray(vertex_array);
    glDrawArrays(GL_LINES, 0, vertices.size() / 2);
}

void GridVisual::draw_panning_locator(Shader& shader, glm::vec3 position) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(0.05f));

    shader.bind(); 
    shader.set_mat4x4("model", model);
    shader.set_vec3("object_color", panning_locator_color);
    panning_locator_mesh.draw(shader, GL_TRIANGLES);
}