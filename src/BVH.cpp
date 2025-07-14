#include "BVH.hpp"

#include <algorithm>
#include <limits>
#include <iostream>
#include <format>

BVH::BVH(std::shared_ptr<Surface> surface, int max_depth) :
    bvh(BVHNode(surface, 0)), surface(surface), max_depth(max_depth)
{
    for (int i = 0; i < surface->triangles.size() / 3; i++) {
        bvh.triangles.push_back({
            surface->triangles[i * 3 + 0],
            surface->triangles[i * 3 + 1],
            surface->triangles[i * 3 + 2]
        });
    }
    bvh.update_bounds();
    bvh.leaf = true;
    bvh.divide(1, max_depth);
}

Intersection BVH::get_triangle_intersection() {
    return {glm::vec3(0.0f, 0.0f, 0.0f), 0};
}

int BVH::get_closest_vertex_intersection() {
    return -1;
}

BVHNode::BVHNode(std::shared_ptr<Surface> surface, int depth) :
    surface(surface), depth(depth)
{
    leaf = true;
}

void BVHNode::split() {
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
    child_a = std::make_unique<BVHNode>(surface, depth+1);
    child_b = std::make_unique<BVHNode>(surface, depth+1);

    for (int i = triangles.size()-1; i >= 0; i--) {
        Triangle triangle = triangles[i];

        float triangle_center = 0.0f;
        for (int j = 0; j < 3; j++) triangle_center += surface->vertices[triangle[j]][largest_axis];
        triangle_center /= 3.0f;

        if (triangle_center < box_center) {
            child_a->triangles.push_back(triangle);
        } else {
            child_b->triangles.push_back(triangle);
        }

        triangles.pop_back();
    }

    child_a->update_bounds();
    child_b->update_bounds();
}

void BVHNode::divide(int depth, int max_depth) {
    if (depth == max_depth) return;

    split();
    child_a->divide(depth+1, max_depth); 
    child_b->divide(depth+1, max_depth);
}

void BVHNode::update_bounds() {
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
    min_x = min_y = min_z = std::numeric_limits<float>::max();
    max_x = max_y = max_z = std::numeric_limits<float>::min();

    for (int i = 0; i < triangles.size(); i++) {
        for (int j = 0; j < 3; j++) {
            min_x = std::min(min_x, surface->vertices[triangles[i][j]].x);
            min_y = std::min(min_y, surface->vertices[triangles[i][j]].y);
            min_z = std::min(min_z, surface->vertices[triangles[i][j]].z);

            max_x = std::max(max_x, surface->vertices[triangles[i][j]].x);
            max_y = std::max(max_y, surface->vertices[triangles[i][j]].y);
            max_z = std::max(max_z, surface->vertices[triangles[i][j]].z);
        }
    }

    point_a = glm::vec3(min_x, min_y, min_z);
    point_b = glm::vec3(max_x, max_y, max_z);

    std::cout << std::format("Depth {}: a = ({}, {}, {}), b = ({}, {}, {})\n", depth, point_a.x, point_a.y, point_a.z, point_b.x, point_b.y, point_b.z);
}