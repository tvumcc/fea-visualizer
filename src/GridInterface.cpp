#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "GridInterface.hpp"

#include <vector>

GridInterface::GridInterface() {
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

/**
 * Render the grid visual to the screen and optionally the panning locator.
 * 
 * @param camera The camera whose orbit point to draw (if draw_panning_locator is true) 
 * @param draw_panning_locator Whether or not to draw the panning locator. This is usually set to true when the user is in the process of panning.
 */
void GridInterface::draw(std::shared_ptr<Camera> camera, bool draw_panning_locator) {
    // Draw Grid Lines
    vertex_color_shader->bind();
    vertex_color_shader->set_mat4x4("model", glm::mat4(1.0f));
    glBindVertexArray(vertex_array);
    glDrawArrays(GL_LINES, 0, vertices.size() / 2);

    // Draw Panning Locator
    if (draw_panning_locator) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, camera->get_orbit_position());
        model = glm::scale(model, glm::vec3(0.05f));

        solid_color_shader->bind(); 
        solid_color_shader->set_mat4x4("model", model);
        solid_color_shader->set_mat4x4("view_proj", camera->get_view_projection_matrix());
        solid_color_shader->set_vec3("object_color", panning_locator_color);
        sphere_mesh->draw(*solid_color_shader, GL_TRIANGLES);
    }
}