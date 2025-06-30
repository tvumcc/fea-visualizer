#include <glad/glad.h>
#include <triangle/triangle.h>

#include <Surface.hpp>

#include <iostream>

#define TRI_LIBRARY

bool Surface::init_from_PSLG(PSLG& pslg) {
    if (pslg.closed()) {
        clear();
        std::vector<double> in_vertices(pslg.vertices.size() * 2, 0.0);
        for (int i = 0; i < in_vertices.size(); i += 2) {
            in_vertices[i] = pslg.vertices[i/2].x;
            in_vertices[i+1] = pslg.vertices[i/2].z;
        }
        std::vector<double> in_holes; // TODO: initialization when support for holes is added

        perform_triangulation(in_vertices.data(), pslg.vertices.size(), reinterpret_cast<int*>(pslg.indices.data()),pslg.indices.size() / 2, in_holes.data(), 0);
        closed = false;
        load_buffers();
        return true;
    } else {
        return false;
    }
}

bool Surface::init_from_obj(const char* file_path) {
    clear();
    return true;
}

void Surface::draw() {
    shader->bind();
    shader->set_vec3("object_color", EDGE_COLOR);
    shader->set_mat4x4("model", glm::mat4(1.0f));
    glBindVertexArray(vertex_array);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Surface::clear() {
    vertices.clear();
    triangles.clear();
    on_boundary.clear();
    load_buffers();
}

void Surface::load_buffers() {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(unsigned int), triangles.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
}

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
    on_boundary = std::vector<bool>(tri_out.numberofedges, false);
    for (int i = 0; i < on_boundary.size(); i++)
        on_boundary[i] = tri_out.edgemarkerlist[i];

    if (tri_out.pointlist != NULL) free(tri_out.pointlist);
    if (tri_out.trianglelist!= NULL) free(tri_out.trianglelist);
    if (tri_out.segmentlist != NULL) free(tri_out.segmentlist);
    if (tri_out.segmentmarkerlist != NULL) free(tri_out.segmentmarkerlist);
    if (tri_out.edgelist != NULL) free(tri_out.edgelist);
    if (tri_out.edgemarkerlist != NULL) free(tri_out.edgemarkerlist);
}