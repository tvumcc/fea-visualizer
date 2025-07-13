#pragma once
#include <glm/glm.hpp>
#include <triangle/triangle.h>

#include "Shader.hpp"
#include "PSLG.hpp"
#include "ColorMap.hpp"

#include <vector>
#include <array>
#include <memory>

/**
 * Represents a triangulated surface, that can be either planar or closed, that exists in 3D space.
 */
class Surface {
public:
    std::vector<glm::vec3> vertices;
    std::vector<float> values;
    std::vector<unsigned int> triangles;
    std::vector<bool> on_boundary;

    std::shared_ptr<Shader> shader;
    std::shared_ptr<Shader> fem_mesh_shader;
    std::shared_ptr<Mesh> sphere_mesh;
    std::shared_ptr<ColorMap> color_map;

    int num_boundary_points = 0;
    bool closed;
    bool initialized = false;
    const glm::vec3 EDGE_COLOR = glm::vec3(0.9f, 0.9f, 0.9f);

    bool init_from_PSLG(PSLG& pslg);
    bool init_from_obj(const char* file_path);

    void draw(bool wireframe);
    void clear();
    void brush(glm::vec3 world_ray, glm::vec3 origin, float value);
private:
    unsigned int vertex_buffer, value_buffer, element_buffer, vertex_array;

    void load_buffers();
    void load_value_buffer();
    void perform_triangulation(double* vertices, int num_vertices, int* segments, int num_segments, double* holes, int num_holes);
};