#include <glad/glad.h>
#include <triangle/triangle.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Utils/Surface.hpp"
#include "Utils/ColorMap.hpp"

#include <unordered_map>
#include <fstream>
#include <format>
#include <filesystem>
#include <iostream>

// This define is needed so that triangle (the triangulation library this project uses) will function as a library that is callable from code.
#define TRI_LIBRARY

/**
 * Initialize this surface with a PSLG via triangulation.
 * This will automatically cause the PSLG to become open.
 * 
 * @param pslg The PSLG to intialize this surface with.
 */
void Surface::init_from_PSLG(PSLG& pslg) {
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
    }
}

/**
 * Initialize this surface with a .obj file.
 * The surface will then become either open or closed depending on the geometry of the mesh. 
 * 
 * @param file_path The path to the .obj file with which to initialize the surface.
 */
void Surface::init_from_obj(const char* file_path) {
    if (!std::filesystem::exists(file_path))
        throw std::runtime_error("A valid file was not provided.");

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;

    if (!reader.ParseFromFile(file_path, reader_config))
        if (!reader.Error().empty())
            throw std::runtime_error(std::format("Error parsing file: {}", reader.Error()));

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    if (attrib.vertices.size() == 0)
        throw std::runtime_error(std::format("File {} does not contain a valid .obj mesh.", file_path));
    if (attrib.normals.size() == 0)
        throw std::runtime_error(std::format("File {} does not contain normals!", file_path));
    
    clear();
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
}

/**
 * Export the vertex positions of this surface to a .obj file
 */
void Surface::export_to_obj(const char* file_path, float vertex_extrusion) {
    std::ofstream of(file_path);

    for (int i = 0; i < vertices.size(); i++) {
        glm::vec3 v = values[i] * std::max(0.0f, vertex_extrusion) * normals[i] + vertices[i];
        glm::vec3 color = color_map->get_color(values[i]);
        of << std::format("v {} {} {} {} {} {}\n", v.x, v.y, v.z, color.r, color.g, color.b);
    }
    
    std::vector<glm::vec3> recalculated_normals(normals.size(), glm::vec3(0.0f));
    
    for (Triangle triangle : triangles) {
        int a = triangle.idx_a;
        int b = triangle.idx_b;
        int c = triangle.idx_c;
        
        glm::vec3 vertex_a = values[a] * std::max(0.0f, vertex_extrusion) * normals[a] + vertices[a];
        glm::vec3 vertex_b = values[b] * std::max(0.0f, vertex_extrusion) * normals[b] + vertices[b];
        glm::vec3 vertex_c = values[c] * std::max(0.0f, vertex_extrusion) * normals[c] + vertices[c];
        
        glm::vec3 normal = glm::normalize(glm::cross(vertex_a - vertex_b, vertex_a - vertex_c));
        
        recalculated_normals[a] += normal;
        recalculated_normals[b] += normal;
        recalculated_normals[c] += normal;
    }
    
    for (int i = 0; i < recalculated_normals.size(); i++) {
        glm::vec3 normal = glm::normalize(recalculated_normals[i]);
        of << std::format("vn {} {} {}\n", normal.x, normal.y, normal.z);
    }
    
    for (Triangle triangle : triangles) {
        of << std::format("f {}//{} {}//{} {}//{}\n",
            triangle[0]+1, triangle[0]+1,
            triangle[1]+1, triangle[1]+1,
            triangle[2]+1, triangle[2]+1
        );
    }

    of.close();
}

/**
 * Export the vertex positions of this surface to a .ply file
 */
void Surface::export_to_ply(const char* file_path, float vertex_extrusion) {
    std::ofstream of(file_path);

    float threshold = 0.1f;
    
    bool mirror = false;
    bool project_down = true;

    std::vector<int> cases;
    std::vector<glm::vec3> clipped_positions;
    std::vector<float> clipped_values;
    std::unordered_map<glm::vec3, int, std::hash<glm::vec3>> clipped_position_map;
    std::vector<Triangle> clipped_triangles;

    for (Triangle triangle : triangles) {
        int a = triangle.idx_a;
        int b = triangle.idx_b;
        int c = triangle.idx_c;

        bool a_above = (values[a] - threshold) > 1e-4;
        bool b_above = (values[b] - threshold) > 1e-4;
        bool c_above = (values[c] - threshold) > 1e-4;
        int count = a_above + b_above + c_above;
        
        auto add_clipped_position = [&](glm::vec3 vec, float value) {
            if (!clipped_position_map.contains(vec)) {
                clipped_position_map[vec] = clipped_positions.size();
                clipped_positions.push_back(vec);
                clipped_values.push_back(value);
            }
        };

        auto extruded_vertex = [&](int idx){
            return values[idx] * std::max(0.0f, vertex_extrusion) * normals[idx] + vertices[idx];
        };

        switch (count) {
            case 1: {
                // The one local index (0, 1, or 2) of the one vertex that is above the discard threshold
                int above_idx = a_above ? 0 : b_above ? 1 : 2;
                Triangle clipped_triangle;
                    
                // TODO: check for div by 0 in glm::mix
                for (int i = 0; i < 3; i++) {
                    glm::vec3 position = i != above_idx
                        ? glm::mix(extruded_vertex(triangle[i]), extruded_vertex(triangle[above_idx]), (threshold - values[triangle[i]]) / (values[triangle[above_idx]] - values[triangle[i]]))
                        : extruded_vertex(triangle[i]);
                    float value = i != above_idx ? threshold : values[triangle[above_idx]];

                    add_clipped_position(position, value);
                    clipped_triangle[i] = clipped_position_map[position];
                }
                
                clipped_triangles.push_back(clipped_triangle);
                
                if (mirror) {
                    
                } else if (project_down) {
                    Triangle projected;
                    for (int i = 0; i < 3; i++) {
                        if (i != above_idx) {
                            projected[i] = clipped_triangle[i];
                        } else {
                            glm::vec3 new_pos = (threshold * vertex_extrusion) * normals[triangle[i]] + vertices[triangle[i]];
                            add_clipped_position(new_pos, threshold);
                            projected[i] = clipped_position_map[new_pos];
                        }
                    }
                    clipped_triangles.push_back(projected);
                }
            } break;
            case 2: {
                // These are the local indices (0, 1, or 2) of the two vertices that are above the discard threshold, ordered by winding order
                int above_idx_1 = a_above ? 0 : 1;
                int above_idx_2 = a_above && b_above ? 1 : 2;
                int below_idx = 3 - (above_idx_1 + above_idx_2);
                
                // TODO: check for div by 0 in glm::mix
                glm::vec3 above_pos_1 = extruded_vertex(triangle[above_idx_1]);
                glm::vec3 above_pos_2 = extruded_vertex(triangle[above_idx_2]);
                glm::vec3 between_pos_1 = glm::mix(extruded_vertex(triangle[below_idx]), above_pos_1, (threshold - values[triangle[below_idx]]) / (values[triangle[above_idx_1]] - values[triangle[below_idx]]));
                glm::vec3 between_pos_2 = glm::mix(extruded_vertex(triangle[below_idx]), above_pos_2, (threshold - values[triangle[below_idx]]) / (values[triangle[above_idx_2]] - values[triangle[below_idx]]));
                
                add_clipped_position(above_pos_1, values[triangle[above_idx_1]]);
                add_clipped_position(above_pos_2, values[triangle[above_idx_2]]);
                add_clipped_position(between_pos_1, threshold);
                add_clipped_position(between_pos_2, threshold);
                
                clipped_triangles.emplace_back(clipped_position_map[between_pos_1], clipped_position_map[above_pos_1], clipped_position_map[between_pos_2]);
                clipped_triangles.emplace_back(clipped_position_map[between_pos_2], clipped_position_map[above_pos_1], clipped_position_map[above_pos_2]);
                
                if (mirror) {
                    
                } else if (project_down) {
                    glm::vec3 above_pos_1_projected = (threshold * vertex_extrusion) * normals[triangle[above_idx_1]] + vertices[triangle[above_idx_1]];
                    glm::vec3 above_pos_2_projected = (threshold * vertex_extrusion) * normals[triangle[above_idx_2]] + vertices[triangle[above_idx_2]];
                    add_clipped_position(above_pos_1_projected, threshold);
                    add_clipped_position(above_pos_2_projected, threshold);
                    clipped_triangles.emplace_back(clipped_position_map[between_pos_1], clipped_position_map[above_pos_1_projected], clipped_position_map[between_pos_2]);
                    clipped_triangles.emplace_back(clipped_position_map[between_pos_2], clipped_position_map[above_pos_1_projected], clipped_position_map[above_pos_2_projected]);
                }
            } break;
            case 3: {
                Triangle clipped_triangle;
                for (int i = 0; i < 3; i++) {
                    glm::vec3 position = extruded_vertex(triangle[i]);
                    add_clipped_position(position, values[triangle[i]]);
                    clipped_triangle[i] = clipped_position_map[position];
                }
                clipped_triangles.push_back(clipped_triangle);
                
                if (mirror) {
                    
                } else if (project_down) {
                    Triangle projected;
                    for (int i = 0; i < 3; i++) {
                        glm::vec3 new_pos = (threshold * vertex_extrusion) * normals[triangle[i]] + vertices[triangle[i]];
                        add_clipped_position(new_pos, threshold);
                        projected[i] = clipped_position_map[new_pos];
                    }
                    clipped_triangles.push_back(projected);
                }
            } break;
        }
    }

    of << "ply\n";
    of << "format ascii 1.0\n";
    of << std::format("element vertex {}\n", clipped_positions.size());
    of << "property float x\n";
    of << "property float y\n";
    of << "property float z\n";
    of << "property float red\n";
    of << "property float green\n";
    of << "property float blue\n";
    of << std::format("element face {}\n", clipped_triangles.size());
    of << "property list uchar uint vertex_indices\n";
    of << "end_header\n";

    for (int i = 0; i < clipped_positions.size(); i++) {
        glm::vec3 position = clipped_positions[i];
        glm::vec3 color = color_map->get_color(clipped_values[i]);
        of << std::format("{} {} {} {} {} {}\n",
            position.x, position.y, position.z,
            color.r, color.g, color.b
        );
    }

    for (Triangle triangle : clipped_triangles) {
        of << std::format("3 {} {} {}\n", triangle.idx_a, triangle.idx_b, triangle.idx_c);
    }

    std::cout << "Finished writing to file\n";

    of.close();
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
 * @param num_vertices The number of vertices. This is the number of elements in vertices divided by 3.
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
    tri_in.pointmarkerlist = nullptr;

    tri_in.segmentlist = segments;
    tri_in.numberofsegments = num_segments;
    tri_in.segmentmarkerlist = nullptr;

    tri_in.holelist = holes;
    tri_in.numberofholes = num_holes;
    tri_in.regionlist = nullptr;
    tri_in.numberofregions = 0;

    triangulateio tri_out = {};
    tri_out.pointlist = nullptr;
    tri_out.trianglelist = nullptr;
    tri_out.segmentlist = nullptr;
    tri_out.segmentmarkerlist = nullptr;
    
    char args[] = "Qeqpza0.005";
    triangulate(args, &tri_in, &tri_out, nullptr);

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

    if (tri_out.pointlist != nullptr) free(tri_out.pointlist);
    if (tri_out.trianglelist!= nullptr) free(tri_out.trianglelist);
    if (tri_out.segmentlist != nullptr) free(tri_out.segmentlist);
    if (tri_out.segmentmarkerlist != nullptr) free(tri_out.segmentmarkerlist);
    if (tri_out.edgelist != nullptr) free(tri_out.edgelist);
    if (tri_out.edgemarkerlist != nullptr) free(tri_out.edgemarkerlist);
}