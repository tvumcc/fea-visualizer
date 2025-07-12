#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "PSLG.hpp"

#include <iostream>

PSLG::PSLG() {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &vertex_buffer); 
    glGenBuffers(1, &element_buffer);

    load_buffers();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
}

/**
 * Load the OpenGL buffers with vertex and index data.
 */
void PSLG::load_buffers() {
    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}

/**
 * Draw the PSLG as a collections of lines on the XZ plane.
 */
void PSLG::draw() {
    if (pending_point.has_value()) {
        vertices.push_back(pending_point.value());
        if (vertices.size() - section_start_idx >= 2) {
            indices.push_back(vertices.size()-2);
            indices.push_back(vertices.size()-1);
        }
    }

    load_buffers();

    shader->bind();
    shader->set_vec3("object_color", EDGE_COLOR);
    shader->set_mat4x4("model", glm::mat4(1.0f));
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);

    if (pending_point.has_value()) {
        if (vertices.size() - section_start_idx >= 2 && indices.size() >= 2) {
            indices.pop_back();
            indices.pop_back();
        }
        vertices.pop_back();
    }

    for (glm::vec3 hole : holes) {
        shader->bind();
        shader->set_vec3("object_color", HOLE_COLOR);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, hole);
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
        shader->set_mat4x4("model", model);
        sphere_mesh->draw(*shader, GL_TRIANGLES);
    }
}
/**
 * Set the pending point, a point which indicates where the next point might be added.
 * This is used so that the user can see where the next point will land as they move their mouse around.
 * 
 * @param The point to set as the new pending point.
 */
void PSLG::set_pending_point(glm::vec3 point) {
    pending_point = point;
}
/**
 * Add the current pending point to the list of points that define the PSLG.
 */
void PSLG::add_pending_point() {
    if (vertices.empty() || 
    (pending_point.has_value() && 
    (std::abs(vertices[vertices.size()-1].x - pending_point.value().x) > 1e-9) &&
    (std::abs(vertices[vertices.size()-1].z - pending_point.value().z) > 1e-9))) {
        vertices.push_back(pending_point.value());
        if (vertices.size() - section_start_idx >= 2) {
            indices.push_back(vertices.size()-2);
            indices.push_back(vertices.size()-1);
        }
    }
}

/** 
 * Add a hole indicator at the specified location.
 * This specifies a closed region that the triangulator will ignore and therefore not triangulate.
 * 
 * @param hole The position at which to denote a closed region to be ignored by the triangulator.
 */
void PSLG::add_hole(glm::vec3 hole) {
    holes.push_back(hole);
}

/**
 * Undo the last added point to PSLG that has not been finalized. 
 */
void PSLG::remove_last_unfinalized_point() {
    if (vertices.size() > section_start_idx) {
        if (vertices.size() - section_start_idx >= 2) {
            indices.pop_back();
            indices.pop_back();
        }
        vertices.pop_back();
    }
}

/**
 * Finalize a section of lines to be added to the PSLG.
 * Once finalized, points cannot be removed other than by clearing the entire PSLG.
 * Additionally, sections are connected but individual sections are disjoint.
 */
void PSLG::finalize() {
    if (vertices.size() - section_start_idx > 0) {
        indices.push_back(vertices.size()-1);
        indices.push_back(section_start_idx);
        section_start_idx = vertices.size();
        pending_point.reset();
    }
}

/**
 * Reset the PSLG by clearing all of the data.
 */
void PSLG::clear() {
    vertices.clear();
    indices.clear();
    holes.clear();
    pending_point.reset();
    section_start_idx = 0;
    load_buffers();
}

/**
 * Remove all of the hole indicators associated with this PSLG.
 */
void PSLG::clear_holes() {
    holes.clear();
}

/**
 * Return whether or not the PSLG is closed, meaning that there are no unfinalized sections and at least one finalized section with at least 3 vertices.
 */
bool PSLG::closed() {
    return section_start_idx == vertices.size() && vertices.size() >= 3;
}