#include "Utils/BVH.hpp"

#include <algorithm>
#include <limits>
#include <iostream>
#include <format>

BVH::BVH(std::shared_ptr<Surface> surface, int max_depth) :
    root(BVHNode(surface, 0)), surface(surface), max_depth(max_depth)
{
    for (int i = 0; i < surface->triangles.size(); i++)
        root.triangle_indices.push_back(i);
    root.update_bounds();
    root.leaf = true;
    root.divide(1, max_depth);
}

/**
 * Computes ray-triangle intersection with the mesh by first using BVH optimization and ray-AABB intersection.
 * 
 * @param origin The starting point of the ray.
 * @param direction The direction of the ray.
 */
RayTriangleIntersection BVH::ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction) {
    std::vector<RayTriangleIntersection> intersections;
    ray_triangle_intersection(origin, direction, root, intersections);

    RayTriangleIntersection out;
    for (RayTriangleIntersection intersection : intersections) {
        if (intersection.tri_idx != -1 && (out.tri_idx == -1 || intersection.distance < out.distance)) {
            out = intersection;
        }
    }
    return out;
}

/**
 * Helper method for the public ray_triangle_intersection method that recursively
 * traverses the child BVHNodes and then computes ray-triangle intersections on all
 * of the leaves while adding the triangle intersections to a vector.
 * 
 * @param origin The starting point of the ray.
 * @param direction The direction of the ray.
 * @param node The BVHNode that is currently being processed.
 * @param intersections A dynamic array of the most successful ray-triangle intersections in each BVHNode leaf that is processed.
 */
void BVH::ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction, BVHNode& node, std::vector<RayTriangleIntersection>& intersections) {
    if (node.ray_aabb_intersection(origin, direction).hit()) {
        if (node.leaf) {
            intersections.push_back(node.ray_triangle_intersection(origin, direction));
        } else {
            ray_triangle_intersection(origin, direction, *node.child_a, intersections); 
            ray_triangle_intersection(origin, direction, *node.child_b, intersections); 
        }
    }
}

BVHNode::BVHNode(std::shared_ptr<Surface> surface, int depth) :
    surface(surface), depth(depth)
{
    leaf = true;
}

/**
 * Splits this BVHNode into two child nodes along its longest axis.
 * All the triangle indices are removed from this node and transferred to its children.
 */
void BVHNode::split() {
    if (degenerate) return;

    int largest_axis = 0;
    float largest_dimension = 0.0f;
    for (int i = 0; i < 3; i++) {
        if (std::abs(point_a[i] - point_b[i]) > largest_dimension) {
            largest_axis = i;
            largest_dimension = std::abs(point_a[i] - point_b[i]);
        }
    }

    float box_center = (point_a[largest_axis] + point_b[largest_axis]) / 2.0f;

    leaf = false;
    child_a = std::make_shared<BVHNode>(surface, depth+1);
    child_b = std::make_shared<BVHNode>(surface, depth+1);

    for (int i = triangle_indices.size()-1; i >= 0; i--) {
        Triangle triangle = surface->triangles[triangle_indices[i]];

        float triangle_center = 0.0f;
        for (int j = 0; j < 3; j++) triangle_center += surface->vertices[triangle[j]][largest_axis];
        triangle_center /= 3.0f;

        if (triangle_center < box_center) {
            child_a->triangle_indices.push_back(triangle_indices[i]);
        } else {
            child_b->triangle_indices.push_back(triangle_indices[i]);
        }

        triangle_indices.pop_back();
    }

    child_a->update_bounds();
    child_b->update_bounds();
}

/**
 * Recursively divides this BVHNode towards a specified maximum depth.
 * 
 * @param depth The current depth that is being processed.
 * @param max_depth The maximum depth at which the recursion will halt.
 */
void BVHNode::divide(int depth, int max_depth) {
    if (depth == max_depth) return;

    split();
    if (!child_a->degenerate) child_a->divide(depth+1, max_depth); 
    if (!child_b->degenerate) child_b->divide(depth+1, max_depth);
}

/**
 * Updates the bounds of this BVHNode to encompass all of its triangles' vertices.
 */
void BVHNode::update_bounds() {
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
    min_x = min_y = min_z = std::numeric_limits<float>::max();
    max_x = max_y = max_z = std::numeric_limits<float>::min();

    for (int i = 0; i < triangle_indices.size(); i++) {
        for (int j = 0; j < 3; j++) {
            min_x = std::min(min_x, surface->vertices[surface->triangles[triangle_indices[i]][j]].x);
            min_y = std::min(min_y, surface->vertices[surface->triangles[triangle_indices[i]][j]].y);
            min_z = std::min(min_z, surface->vertices[surface->triangles[triangle_indices[i]][j]].z);

            max_x = std::max(max_x, surface->vertices[surface->triangles[triangle_indices[i]][j]].x);
            max_y = std::max(max_y, surface->vertices[surface->triangles[triangle_indices[i]][j]].y);
            max_z = std::max(max_z, surface->vertices[surface->triangles[triangle_indices[i]][j]].z);
        }
    }

    if (triangle_indices.size() == 0) {
        point_a = point_b = glm::vec3(0.0f, 0.0f, 0.0f);
        degenerate = true;
    } else {
        point_a = glm::vec3(min_x, min_y, min_z);
        point_b = glm::vec3(max_x, max_y, max_z);
        degenerate = false;
    }
}

/**
 * Computes and returns the intersection of a ray and an AABB.
 * See https://tavianator.com/2011/ray_box.html for the Slab Method implementation.
 * 
 * @param origin The starting point of the ray.
 * @param direction The direction of the ray.
 */
RayAABBIntersection BVHNode::ray_aabb_intersection(glm::vec3 origin, glm::vec3 direction) {
    float t_min = std::numeric_limits<float>::min();
    float t_max = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; i++) {
        float t1 = (point_a[i] - origin[i]) / direction[i];
        float t2 = (point_b[i] - origin[i]) / direction[i];

        t_min = std::max(t_min, std::min(t1, t2));
        t_max = std::min(t_max, std::max(t1, t2));
    }
    
    return {t_min, t_max};
}

/**
 * Computes and returns the intersection of a ray and a triangle.
 * 
 * @param origin The starting point of the ray.
 * @param direction The direction of the ray.
 */
RayTriangleIntersection BVHNode::ray_triangle_intersection(glm::vec3 origin, glm::vec3 direction) {
    RayTriangleIntersection intersection;

    for (int i = 0; i < triangle_indices.size(); i++) {
        Triangle triangle = surface->triangles[triangle_indices[i]];
        glm::vec3 A = surface->vertices[triangle.idx_a];
        glm::vec3 B = surface->vertices[triangle.idx_b];
        glm::vec3 C = surface->vertices[triangle.idx_c];
        glm::vec3 center = (A + B + C) / 3.0f;

        glm::vec3 normal = glm::normalize(glm::cross(B - A, C - A));
        float denom = glm::dot(direction, normal);

        if (std::abs(denom) > 1e-6) {
            float dist = -glm::dot(origin - center, normal) / denom;
            glm::vec3 point = origin + direction * dist;

            int pos = 0, neg = 0;

            for (int j = 0; j < 3; j++) {
                glm::vec3 p1 = surface->vertices[triangle[j]];
                glm::vec3 p2 = surface->vertices[triangle[(j + 1) % 3]];

                glm::vec3 perpVector = glm::cross(normal, p2 - p1);
                if (glm::dot(perpVector, point - p1) < 0.0f) neg++;
                else pos++;
            }

            if ((pos == 3 || neg == 3) && (intersection.tri_idx == -1 || dist < intersection.distance) && dist > 0.0f) {
                intersection.tri_idx = triangle_indices[i];
                intersection.distance = dist;
                intersection.point = point;
            }
        }
    }

    return intersection;
}