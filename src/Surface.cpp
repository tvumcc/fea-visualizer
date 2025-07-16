#include <glad/glad.h>
#include <triangle/triangle.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Surface.hpp"
#include "BVH.hpp"

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <format>

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

        normals = std::vector<glm::vec3>(vertices.size(), glm::vec3(0.0f, 1.0f, 0.0f)); // Normals to the XZ always point in the +Y direction.
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
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;

    if (!reader.ParseFromFile(file_path, reader_config)) {
        if (!reader.Error().empty())
            std::cerr << "[ERROR] TinyObjReader: " << reader.Error();
        exit(1);
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    
    for (int s = 0; s < shapes.size(); s++) {
        int index_offset = 0;
        for (int i = 0; i < attrib.vertices.size() / 3; i++) {
            vertices.push_back(glm::vec3(attrib.vertices[i*3+0], attrib.vertices[i*3+1], attrib.vertices[i*3+2]));
        }

        for (int i = 0; i < shapes[s].mesh.indices.size() / 3; i++) {
            Triangle triangle;
            for (int j = 0; j < 3; j++)
                triangle[j] = shapes[s].mesh.indices[i*3+j].vertex_index;
            triangles.push_back(triangle);
        }
    }

    normals = std::vector<glm::vec3>(vertices.size(), glm::vec3(0.0f, 0.0f, 0.0f));
    for (int s = 0; s < shapes.size(); s++) {
        for (int i = 0; i < shapes[s].mesh.indices.size(); i++) {
            normals[shapes[s].mesh.indices[i].vertex_index] += glm::vec3(
                attrib.normals[shapes[s].mesh.indices[i].normal_index*3+0],
                attrib.normals[shapes[s].mesh.indices[i].normal_index*3+1],
                attrib.normals[shapes[s].mesh.indices[i].normal_index*3+2]
            );
        }
    }
    for (int i = 0; i < normals.size(); i++)
        normals[i] = glm::normalize(normals[i]);

    std::unordered_map<unsigned int, std::unordered_map<unsigned int, int>> edges;
    on_boundary = std::vector<bool>(vertices.size(), false);
    for (int i = 0; i < triangles.size(); i++) {
        for (int j = 0; j < 3; j++) {
            unsigned int idx_a = triangles[i][j];
            unsigned int idx_b = triangles[i][(j + 1) % 3];
            edges[idx_a][idx_b]++;
            edges[idx_b][idx_a]++;
        }
    }

    for (auto a : edges) {
        for (auto b : a.second) {
            if (b.second == 1) {
                on_boundary[a.first] = true;
                on_boundary[b.first] = true;
            } 
        }
    }

    num_boundary_points = 0;
    for (int i = 0; i < on_boundary.size(); i++)
        if (on_boundary[i])
            num_boundary_points++;

    values = std::vector<float>(vertices.size(), 0.0f);
    closed = num_boundary_points == 0;
    initialized = true;
    load_buffers(); 

    return true;
}

/**
 * Renders this surface to the screen.
 */
void Surface::draw(bool wireframe) {
    if (initialized) {
        load_value_buffer();

        // Draw Colored Surface
        fem_mesh_shader->bind();
        fem_mesh_shader->set_mat4x4("model", glm::mat4(1.0f));
        color_map->set_uniforms(*fem_mesh_shader);
        glBindVertexArray(vertex_array);
        glDrawElements(GL_TRIANGLES, triangles.size() * 3, GL_UNSIGNED_INT, 0);

        // Draw Wireframe
        if (wireframe) {
            wireframe_shader->bind();
            wireframe_shader->set_mat4x4("model", glm::mat4(1.0f));
            wireframe_shader->set_vec3("object_color", EDGE_COLOR);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDepthFunc(GL_LEQUAL);
            glDrawElements(GL_TRIANGLES, triangles.size() * 3, GL_UNSIGNED_INT, 0);
            glDepthFunc(GL_LESS);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
    initialized = false;
    load_buffers();
}

/**
 * Resets this surface by only setting all of the nodal values to 0.
 */
void Surface::clear_values() {
    values = std::vector<float>(vertices.size(), 0.0f);
}

/**
 * Returns the total number of vertices minus the number of vertices lying on the boundary of the domain.
 */
int Surface::num_unknown_nodes() {
    return vertices.size() - num_boundary_points;
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

    glGenBuffers(1, &normal_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &value_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, value_buffer);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, value_buffer);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
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

    for (int i = 0; i < tri_out.numberoftriangles; i++) {
        Triangle triangle;
        for (int j = 0; j < 3; j++) {
            triangle[j] = tri_out.trianglelist[i*3+j];
        }
        triangles.push_back(triangle);
    }
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