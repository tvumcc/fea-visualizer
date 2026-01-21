#pragma once
#include <glad/glad.h>

#include "Utils/Shader.hpp"

#include <vector>
#include <string>

struct MeshVertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;

    MeshVertex() {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        uv = glm::vec2(0.0f, 0.0f);
        normal = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    MeshVertex(glm::vec3 position, glm::vec2 uv, glm::vec3 normal) {
        this->position = position;
        this->uv = uv;
        this->normal = normal;
    }
};

/**
 * Represents a collection of vertices that define a 3D geometry.
 * Provides support for the position, uv, and normal components of each vertex.
 */
class Mesh {
public:
    Mesh(const std::vector<MeshVertex>& vertices);
    Mesh(std::string path);
    void draw(Shader& shader, GLenum primitive_type);
    std::vector<MeshVertex> vertices;
private:
    unsigned int vertex_buffer, vertex_array;

    void init_data();
};