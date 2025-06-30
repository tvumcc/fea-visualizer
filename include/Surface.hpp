#pragma once
#include <glm/glm.hpp>
#include <triangle/triangle.h>

#include "Shader.hpp"
#include "PSLG.hpp"

#include <vector>
#include <array>
#include <memory>

class Surface {
public:
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> triangles;
    std::vector<bool> on_boundary;
    std::shared_ptr<Shader> shader;

    bool closed;
    const glm::vec3 EDGE_COLOR = glm::vec3(0.9f, 0.9f, 0.9f);

    Surface();

    void init_from_PSLG(PSLG& pslg);
    void init_from_obj(const char* file_path);

    void draw();
    void clear();
    void load_buffers();
    void perform_triangulation(double* vertices, int num_vertices, int* segments, int num_segments, double* holes, int num_holes);
private:
    unsigned int vertex_buffer, element_buffer, vertex_array;
};