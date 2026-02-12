#pragma once
#include "Utils/Shader.hpp"
#include "Utils/Mesh.hpp"

#include <vector>
#include <optional>
#include <memory>

/**
 * Representation of a Planar Straight Line Graph (PSLG) on the XZ plane.
 * Provides an interface for drawing a PSLG by specifying points and holes with the intent of later being able to triangulate it into a mesh.
 */
class PSLG {
public:
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    std::vector<glm::vec3> holes;
    std::optional<glm::vec3> pending_point;

    std::shared_ptr<Shader> solid_color_shader;
    std::shared_ptr<Shader> textured_shader;
    std::shared_ptr<Mesh> sphere_mesh;
    std::shared_ptr<Mesh> quad_mesh;

    bool show_stencil_image = true;
    bool stencil_initialized = false;
    float stencil_scale = 1.0f;

    float triangle_area = 0.005;

    int section_start_idx = 0;

    const glm::vec3 EDGE_COLOR = glm::vec3(0.9f, 0.9f, 0.9f);
    const glm::vec3 HOLE_COLOR = glm::vec3(0.0f, 0.9f, 0.0f);

    PSLG();

    void draw();
    void draw_stencil_image();
    void set_pending_point(glm::vec3 point);
    void add_pending_point();
    void add_hole(glm::vec3 hole);
    void load_stencil_image(std::string path);
    void remove_last_unfinalized_point();
    void finalize();
    void clear();
    void clear_holes();
    void clear_stencil();
    bool closed();
    bool empty();
private:
    unsigned int vertex_buffer, element_buffer, vertex_array; 

    unsigned int stencil_texture;
    float stencil_aspect_ratio = 1.0f;

    void load_buffers();
};