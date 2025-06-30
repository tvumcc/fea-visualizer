#include <glad/glad.h>

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

void PSLG::load_buffers() {
    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}

void PSLG::draw() {
    if (pending_point.has_value()) {
        vertices.push_back(pending_point.value());
        if (vertices.size() - section_start_idx >= 2) {
            indices.push_back(vertices.size()-2);
            indices.push_back(vertices.size()-1);
        }
    }

    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

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
}
void PSLG::set_pending_point(glm::vec3 point) {
    pending_point = point;
}
void PSLG::add_pending_point() {
    vertices.push_back(pending_point.value());
    if (vertices.size() - section_start_idx >= 2) {
        indices.push_back(vertices.size()-2);
        indices.push_back(vertices.size()-1);
    }
}
void PSLG::remove_last_unfinalized_point() {
    if (vertices.size() > section_start_idx) {
        if (vertices.size() - section_start_idx >= 2) {
            indices.pop_back();
            indices.pop_back();
        }
        vertices.pop_back();
    }
}
void PSLG::finalize() {
    if (vertices.size() - section_start_idx > 0) {
        indices.push_back(vertices.size()-1);
        indices.push_back(section_start_idx);
        section_start_idx = vertices.size();
        pending_point.reset();
    }
}
void PSLG::clear() {
    vertices.clear();
    indices.clear();
    pending_point.reset();
    section_start_idx = 0;
    load_buffers();
}
bool PSLG::closed() {
    return section_start_idx == vertices.size() && vertices.size() >= 3;
}