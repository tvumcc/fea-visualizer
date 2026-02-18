#version 460
layout (local_size_x = 1024) in;

layout (std430, binding = 0) buffer state {
    float r_i_norm;
    float r_i1_norm;
    float r_0_norm;
    float d_iA_dot;

    uint N;
    uint M;
    uint total_nodes;
    float buffer_result[];
};
layout (std430, binding = 1) buffer A_matrix {float A[];}; // Size of 2 * N * M; ELL Formatted: The first N*M elements are the data, the next N*M elements are the indices (-1 for none)
layout (std430, binding = 2) buffer known {float b[];}; // Size of N
layout (std430, binding = 3) buffer unknown {float x[];}; // Size of N
layout (std430, binding = 4) buffer residual {float r[];}; // Size of N
layout (std430, binding = 5) buffer search_direction {float d[];}; // Size of N
layout (std430, binding = 6) buffer idx_map {uint idx_map[];}; // Size of total_nodes
layout (std430, binding = 7) buffer values {float values[];}; // Size of N; Also mapped to a vertex buffer for the rendering pipeline

uniform bool first_pass;
uniform int stage;

shared float shared_data[1024];

void main() {
    int globalID = int(gl_GlobalInvocationID.x);
    int localID = int(gl_LocalInvocationID.x);

    float result = 0.0;
    if (globalID < N) {
        if (first_pass) {
            switch (stage) {
                case 0:
                case 2:
                    result = r[globalID] * r[globalID];
                    break;
                case 1:
                    float matrix_product = 0.0;
                    for (int i = 0; i < M; i++) {
                        int idx = A[(N * M) + (globalID * M + i)];
                        if (idx != -1) matrix_product += A[globalID * M + i] * d[idx];
                        else break;
                    }
                    result = d[globalID] * matrix_product;
                    break;
            }
        } else {
            result = buffer_result[globalID];
        }
    }
    shared_data[localID] = result;
    barrier();

    for (int i = int(gl_WorkGroupSize.x) / 2; i > 0; i >>= 1) {
        if (gl_LocalInvocationID.x < i) {
            shared_data[localID] += shared_data[localID + i];
        }
        barrier();
    }

    if (localID == 0) {
        buffer_result[gl_WorkGroupID.x] = shared_data[localID];

        switch (stage) {
            case 0:
                r_i_norm = buffer_result[gl_WorkGroupID.x];
                break;
            case 1:
                d_iA_dot = buffer_result[gl_WorkGroupID.x];
                break;
            case 2:
                r_i1_norm = buffer_result[gl_WorkGroupID.x];
                break;
        } 
    }
}