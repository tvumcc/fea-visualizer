#pragma once
#include <glm/glm.hpp>

#include "Surface.hpp"

#include <vector>
#include <memory>


struct RayTriangleIntersection {
    glm::vec3 point = glm::vec3(0.0f, 0.0f, 0.0f);
    float distance = 0.0f;
    int tri_idx = -1;
};

struct RayAABBIntersection {
    float t_min;
    float t_max;

    bool hit() {
        return t_max >= t_min;
    }
};

class BVHNode {
public:
    glm::vec3 point_a;
    glm::vec3 point_b;
    std::shared_ptr<BVHNode> child_a;
    std::shared_ptr<BVHNode> child_b;
    int depth;
    bool leaf; // If this is false, then triangles should be empty

    std::shared_ptr<Surface> surface;
    std::vector<unsigned int> triangle_indices;

    BVHNode();
    BVHNode(std::shared_ptr<Surface> surface, int depth);
    void split();
    void divide(int depth, int max_depth);
    void update_bounds();
    RayAABBIntersection ray_aabb_intersection(glm::vec3 origin, glm::vec3 direction);
    RayTriangleIntersection ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction);

    // Rendering
    unsigned int vertex_buffer, vertex_array;
    void draw();
    void load_buffers();
};

class BVH {
public:
    BVHNode bvh;
    std::shared_ptr<Surface> surface;
    int max_depth;

    BVH();
    BVH(std::shared_ptr<Surface> surface, int max_depth);
    RayTriangleIntersection ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction);
    int get_closest_vertex_intersection();

    // Rendering
    std::shared_ptr<Shader> shader;
    void draw(int max_depth);
    void draw(int max_depth, int depth, BVHNode& node);
private:
    void ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction, BVHNode& node, std::vector<RayTriangleIntersection>& intersections);
};