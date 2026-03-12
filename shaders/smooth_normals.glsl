#version 460
layout (local_size_x = 1024) in;

layout (std430, binding = 0) buffer InPositions {float in_positions[][3];};
layout (std430, binding = 1) buffer InNormals {float in_normals[][3];};
layout (std430, binding = 2) buffer InValues {float in_values[];};
layout (std430, binding = 3) buffer Indices {int indices[];}; 
layout (std430, binding = 4) buffer OutNormals {uint out_normals[][3];}; 

uniform int stage;
uniform int num_triangles;
uniform int num_vertices;
uniform float vertex_extrusion;

void atomic_add_float(int vertex_idx, int component, float value) {
    uint new, prev;
    do {
        prev = out_normals[vertex_idx][component];
        new = floatBitsToUint(uintBitsToFloat(prev) + value);
    } while (atomicCompSwap(out_normals[vertex_idx][component], prev, new) != prev);
}

void main() {
    int globalID = int(gl_GlobalInvocationID.x);
    int localID = int(gl_LocalInvocationID.x);

    switch (stage) {
        case 0: {
            if (globalID < num_vertices) {
                for (int i = 0; i < 3; i++) {
                    out_normals[globalID][i] = 0;
                }
            }
        } break;
        case 1: {
            if (globalID < num_triangles) {
                int a_idx = indices[3 * globalID + 0];
                int b_idx = indices[3 * globalID + 1];
                int c_idx = indices[3 * globalID + 2];

                vec3 a_pos = vec3(in_positions[a_idx][0], in_positions[a_idx][1], in_positions[a_idx][2]);
                vec3 b_pos = vec3(in_positions[b_idx][0], in_positions[b_idx][1], in_positions[b_idx][2]);
                vec3 c_pos = vec3(in_positions[c_idx][0], in_positions[c_idx][1], in_positions[c_idx][2]);

                vec3 a_normal = vec3(in_normals[a_idx][0], in_normals[a_idx][1], in_normals[a_idx][2]);
                vec3 b_normal = vec3(in_normals[b_idx][0], in_normals[b_idx][1], in_normals[b_idx][2]);
                vec3 c_normal = vec3(in_normals[c_idx][0], in_normals[c_idx][1], in_normals[c_idx][2]);

                a_pos += min(1.0, vertex_extrusion * max(0.0, in_values[a_idx])) * a_normal;
                b_pos += min(1.0, vertex_extrusion * max(0.0, in_values[b_idx])) * b_normal;
                c_pos += min(1.0, vertex_extrusion * max(0.0, in_values[c_idx])) * c_normal;

                vec3 ab = b_pos - a_pos; 
                vec3 ac = c_pos - a_pos;
                vec3 calculated_normal = cross(ab, ac);

                for (int i = 0; i < 3; i++) {
                    atomic_add_float(a_idx, i, calculated_normal[i]);
                    atomic_add_float(b_idx, i, calculated_normal[i]);
                    atomic_add_float(c_idx, i, calculated_normal[i]);
                }
            }
        } break;
    }
}