#pragma once
#include <glm/glm.hpp>

#include "Shader.hpp"

#include <vector>
#include <optional>
#include <memory>

class PSLG {
public:
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    std::optional<glm::vec3> pending_point;
    std::shared_ptr<Shader> shader;

    int section_start_idx = 0;

    const glm::vec3 EDGE_COLOR = glm::vec3(0.9f, 0.9f, 0.9f);

    PSLG();

    void draw();
    void set_pending_point(glm::vec3 point);
    void add_pending_point();
    void finalize();
    void clear();
private:
    unsigned int vertex_buffer, element_buffer, vertex_array; 
    void load_buffers();
};