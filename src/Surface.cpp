#include <glad/glad.h>
#include <triangle/triangle.h>
#include <glm/gtc/matrix_transform.hpp>

#include <Surface.hpp>

#include <iostream>

// This define is needed so that triangle (the triangulation library this project uses) will function as a library that is callable from code.
#define TRI_LIBRARY

/**
 * Initialize this surface with a PSLG via triangulation.
 * This will automatically cause the PSLG to become open.
 * 
 * @param pslg The PSLG to intialize this surface with.
 */
bool Surface::init_from_PSLG(PSLG& pslg) {
    if (pslg.closed()) {
        clear();
        std::vector<double> in_vertices(pslg.vertices.size() * 2, 0.0);
        for (int i = 0; i < in_vertices.size(); i += 2) {
            in_vertices[i] = pslg.vertices[i/2].x;
            in_vertices[i+1] = pslg.vertices[i/2].z;
        }
        std::vector<double> in_holes(pslg.holes.size() * 2, 0.0); // TODO: initialization when support for holes is added
        for (int i = 0; i < in_holes.size(); i += 2) {
            in_holes[i] = pslg.holes[i/2].x;
            in_holes[i+1] = pslg.holes[i/2].z;
        }

        perform_triangulation(in_vertices.data(), pslg.vertices.size(), reinterpret_cast<int*>(pslg.indices.data()), pslg.indices.size() / 2, in_holes.data(), pslg.holes.size());

        values = std::vector<float>(vertices.size(), 0.0f);
        closed = false;
        initialized = true;
        load_buffers();
        return true;
    } else {
        return false;
    }
}

/**
 * Initialize this surface with a .obj file.
 * The surface will then become either open or closed depending on the geometry of the mesh. 
 * 
 * @param file_path The path to the .obj file with which to initalize the surface.
 */
bool Surface::init_from_obj(const char* file_path) {
    clear();
    return true;
}

/**
 * Renders this surface to the screen.
 */
void Surface::draw() {
    if (initialized) {
        load_value_buffer();

        // Draw Colored Surface
        fem_mesh_shader->bind();
        fem_mesh_shader->set_mat4x4("model", glm::mat4(1.0f));
        color_map->set_uniforms(*fem_mesh_shader);
        glBindVertexArray(vertex_array);
        glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, 0);

        // Draw Wireframe
        shader->bind();
        shader->set_mat4x4("model", glm::mat4(1.0f));
        shader->set_vec3("object_color", EDGE_COLOR);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDepthFunc(GL_LEQUAL);
        glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, 0);
        glDepthFunc(GL_LESS);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // TODO: Switch to instancing for faster rendering
        for (int i = 0; i < vertices.size(); i++) {
            shader->bind();
            shader->set_vec3("object_color", color_map->get_color(values[i]));
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, vertices[i]);
            model = glm::scale(model, glm::vec3(0.02f));
            shader->set_mat4x4("model", model);
            sphere_mesh->draw(*shader, GL_TRIANGLES);
        }
    }
}

/**
 * Set the value of some region on the surface given a world ray and origin.
 * Right now, this sets the value of the closest vertex to the intersection point. 
 * Note that the ray must intersect the surface in order for a value to be set. 
 * 
 * @param world_ray The normalized direction vector of the ray derived from mouse picking.
 * @param origin The origin of the world ray.
 * @param value The value to set each of the nodal values to. 
 */
void Surface::brush(glm::vec3 world_ray, glm::vec3 origin, float value) {
    int tri_idx = -1;
    float min_dist = 0.0f;
    glm::vec3 intersection_point;

    for (int i = 0; i < triangles.size() / 3; i++) {
        glm::vec3 A = vertices[triangles[i*3+0]];
        glm::vec3 B = vertices[triangles[i*3+1]];
        glm::vec3 C = vertices[triangles[i*3+2]];
        glm::vec3 center = (A + B + C) / 3.0f;

        glm::vec3 normal = glm::normalize(glm::cross(B - A, C - A));
        float denom = glm::dot(world_ray, normal);

        if (std::abs(denom) > 1e-6) {
            float dist = -glm::dot(origin - center, normal) / denom;
            glm::vec3 point = origin + world_ray * dist;

            int pos = 0, neg = 0;

            for (int j = 0; j < 3; j++) {
                glm::vec3 p1 = vertices[triangles[i*3+j]];
                glm::vec3 p2 = vertices[triangles[i*3+((j+1) % 3)]];

                glm::vec3 perpVector = glm::cross(normal, p2 - p1);
                if (glm::dot(perpVector, point - p1) < 0.0f) neg++;
                else pos++;
            }

            if ((pos == 3 || neg == 3) && (tri_idx == -1 || dist < min_dist) && dist > 0.0f) {
                tri_idx = i;
                min_dist = dist;
                intersection_point = point;
            }
        }
    }

    if (tri_idx != -1) {
        int point_idx = -1;
        float min_point_dist = 0.0f;
        for (int i = 0; i < 3; i++) {
            float dist = glm::distance(vertices[triangles[tri_idx*3+i]], intersection_point);
            if ((point_idx == -1 || dist < min_point_dist) && dist > 0.0f) {
                min_point_dist = dist;
                point_idx = i;
            }
        }
        
        if (point_idx != -1) {
            values[triangles[tri_idx*3+point_idx]] = value;
        }
    }
}


/**
 * Resets this surface by clearing all of the data associated with it.
 */
void Surface::clear() {
    vertices.clear();
    triangles.clear();
    on_boundary.clear();
    values.clear();
    load_buffers();
}

/**
 * Load all OpenGL buffers (including the value buffer) with their respective data.
 */
void Surface::load_buffers() {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &value_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, value_buffer);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(unsigned int), triangles.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, value_buffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
}

/**
 * Load just the OpenGL buffers associated with the nodal values.
 */
void Surface::load_value_buffer() {
    glBindVertexArray(vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, value_buffer);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STATIC_DRAW);
}

/**
 * Triangulate a PSLG defined by buffers of data.
 * 
 * @param vertices The flattened vertex data as a raw pointer. Every 3 doubles define a 3D position.
 * @param num_vertcies The number of vertices. This is the number of elements in vertices divided by 3.
 * @param segments The flattened line data as a raw pointer. Every pair of integers define a line by indexing 2 vertices.
 * @param num_segments The number of segments. The is the number of elements in segments divided by 2.
 * @param holes The flattened hole data as a raw pointer. Every 3 double define a 3D position that denote a closed region as a hole.
 * @param num_holes The number of holes. This is the number of element in holes divided by 3.
 */
void Surface::perform_triangulation(double* vertices, int num_vertices, int* segments, int num_segments, double* holes, int num_holes) {
    triangulateio tri_in = {};
    tri_in.pointlist = vertices;
    tri_in.numberofpoints = num_vertices;
    tri_in.numberofpointattributes = 0;
    tri_in.pointmarkerlist = NULL;

    tri_in.segmentlist = segments;
    tri_in.numberofsegments = num_segments;
    tri_in.segmentmarkerlist = NULL;

    tri_in.holelist = holes;
    tri_in.numberofholes = num_holes;
    tri_in.regionlist = NULL;
    tri_in.numberofregions = 0;

    triangulateio tri_out = {};
    tri_out.pointlist = NULL;
    tri_out.trianglelist = NULL;
    tri_out.segmentlist = NULL;
    tri_out.segmentmarkerlist = NULL;
    
    char args[] = "eqpza0.05";
    triangulate(args, &tri_in, &tri_out, NULL);

    this->vertices = std::vector<glm::vec3>(tri_out.numberofpoints, glm::vec3(0.0));
    for (int i = 0; i < tri_out.numberofpoints; i++) {
        this->vertices[i] = glm::vec3(
            tri_out.pointlist[i*2],
            0.0,
            tri_out.pointlist[i*2+1]
        );
    }

    triangles = std::vector<unsigned int>(tri_out.numberoftriangles * 3, 0);
    for (int i = 0; i < triangles.size(); i++)
        triangles[i] = tri_out.trianglelist[i]; 
    on_boundary = std::vector<bool>(tri_out.numberofpoints, false);
    num_boundary_points = 0;
    for (int i = 0; i < on_boundary.size(); i++) {
        on_boundary[i] = tri_out.pointmarkerlist[i];
        if (on_boundary[i])
            num_boundary_points++;
    }

    if (tri_out.pointlist != NULL) free(tri_out.pointlist);
    if (tri_out.trianglelist!= NULL) free(tri_out.trianglelist);
    if (tri_out.segmentlist != NULL) free(tri_out.segmentlist);
    if (tri_out.segmentmarkerlist != NULL) free(tri_out.segmentmarkerlist);
    if (tri_out.edgelist != NULL) free(tri_out.edgelist);
    if (tri_out.edgemarkerlist != NULL) free(tri_out.edgemarkerlist);
}