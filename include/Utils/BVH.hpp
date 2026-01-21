#pragma once
#include <glm/glm.hpp>

#include "Utils/Surface.hpp"

#include <vector>
#include <memory>

/**
 * Stores data for the intersection of a ray and a triangle.
 * tri_idx is an index into a Surface object, if it is -1 then the ray did not intersect a triangle.
 */
struct RayTriangleIntersection {
    glm::vec3 point = glm::vec3(0.0f, 0.0f, 0.0f);
    float distance = 0.0f;
    int tri_idx = -1;
};

/**
 * Stores data for the intersection of a ray and an AABB.
 */
struct RayAABBIntersection {
    float t_min;
    float t_max;

    bool hit() {
        return t_max >= t_min;
    }
};

/**
 * A bounding box node for use by a BVH.
 * These effectively form a tree that partitions 3D space.
 */
class BVHNode {
public:
    glm::vec3 point_a;
    glm::vec3 point_b;
    std::shared_ptr<BVHNode> child_a;
    std::shared_ptr<BVHNode> child_b;

    int depth;
    bool leaf; // If this is false, then triangles should be empty
    bool degenerate = false; // If this is true, then there are no triangles in this bounding box

    std::shared_ptr<Surface> surface;
    std::vector<unsigned int> triangle_indices;

    BVHNode(std::shared_ptr<Surface> surface, int depth);
    void split();
    void divide(int depth, int max_depth);
    void update_bounds();
    RayAABBIntersection ray_aabb_intersection(glm::vec3 origin, glm::vec3 direction);
    RayTriangleIntersection ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction);
};

/**
 * Manages a Bounding Volume Hierarchy (BVH) to optimize the computation of
 * ray intersections with a set of triangles from O(n) to O(log n) time.  
 */
class BVH {
public:
    BVHNode root;
    std::shared_ptr<Surface> surface;
    int max_depth;

    BVH(std::shared_ptr<Surface> surface, int max_depth);
    RayTriangleIntersection ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction);
private:
    void ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction, BVHNode& node, std::vector<RayTriangleIntersection>& intersections);
};