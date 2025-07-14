#pragma once
#include <glm/glm.hpp>

#include "Surface.hpp"

#include <vector>
#include <memory>

struct Triangle {
    unsigned int idx_a;
    unsigned int idx_b;
    unsigned int idx_c;

    unsigned int operator[](int idx) {
        switch (idx) {
            case 0: return idx_a;
            case 1: return idx_b;
            default: return idx_c;
        }
    }
};

struct Intersection {
    glm::vec3 point;
    unsigned int tri_idx;
};

class BVHNode {
public:
    glm::vec3 point_a;
    glm::vec3 point_b;
    std::unique_ptr<BVHNode> child_a;
    std::unique_ptr<BVHNode> child_b;
    int depth;
    bool leaf; // If this is false, then triangles should be empty

    std::shared_ptr<Surface> surface;
    std::vector<Triangle> triangles;

    BVHNode();
    BVHNode(std::shared_ptr<Surface> surface, int depth);
    void split();
    void divide(int depth, int max_depth);
    void update_bounds();
};

class BVH {
public:
    BVHNode bvh;
    std::shared_ptr<Surface> surface;
    int max_depth;

    BVH();
    BVH(std::shared_ptr<Surface> surface, int max_depth);
    Intersection get_triangle_intersection();
    int get_closest_vertex_intersection();
};