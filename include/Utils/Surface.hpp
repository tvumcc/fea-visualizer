#pragma once
#include <glm/glm.hpp>

#include "Utils/Shader.hpp"
#include "Utils/PSLG.hpp"
#include "Utils/ColorMap.hpp"

#include <vector>
#include <memory>

struct Triangle {
    unsigned int idx_a;
    unsigned int idx_b;
    unsigned int idx_c;
    
    unsigned int& operator[](int idx) {
        switch (idx) {
            case 0: return idx_a;
            case 1: return idx_b;
            default: return idx_c;
        }
    }
};

enum class MeshType {
    Open = 0,
    Closed,
    Mirrored,
};

/**
 * Represents a triangulated surface, that can be either planar or closed, that exists in 3D space.
 */
class Surface {
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<float> values;
    std::vector<Triangle> triangles;
    std::vector<bool> on_boundary;

    std::shared_ptr<Shader> wireframe_shader;
    std::shared_ptr<Shader> fem_mesh_shader;
    std::shared_ptr<ColorMap> color_map;

    int num_boundary_points = 0;
    bool closed;
    bool initialized = false;
    const glm::vec3 EDGE_COLOR = glm::vec3(0.9f, 0.9f, 0.9f);

    void init_from_PSLG(PSLG& pslg);
    void init_from_obj(const char* file_path);
    void export_to_ply(const char* file_path, float vertex_extrusion = 0.25f, float threshold = 0.0f, MeshType mesh_type = MeshType::Open);

    void draw(bool wireframe);
    void clear();
    void clear_values();

    int num_unknown_nodes();
private:
    unsigned int vertex_buffer, value_buffer, normal_buffer, element_buffer, vertex_array;

    void load_buffers();
    void load_value_buffer();
    void perform_triangulation(double* vertices, int num_vertices, int* segments, int num_segments, double* holes, int num_holes, float triangle_area);
};