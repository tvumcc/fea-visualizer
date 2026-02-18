#version 460
layout (local_size_x = 1024) in;

layout (std430, binding = 0) buffer state {
    float r_i_norm;
    float r_i1_norm;
    float r_0_norm;
    float d_iA_norm;

    uint N;
    uint M;
    uint total_nodes;
    float result[];
};

layout (std430, binding = 1) buffer A_matrix {float A[];}; // Size of 2 * N * M; ELL Formatted
layout (std430, binding = 2) buffer known {float b[];}; // Size of N
layout (std430, binding = 3) buffer unknown {float x[];}; // Size of N
layout (std430, binding = 4) buffer residual {float r[];}; // Size of N
layout (std430, binding = 5) buffer search_direction {float d[];}; // Size of N
layout (std430, binding = 6) buffer idx_map {uint idx_map[];}; // Size of total_nodes
layout (std430, binding = 7) buffer values {float values[];}; // Size of N; Also mapped to a vertex buffer for the rendering pipeline

uniform bool first_pass;
uniform int brush_idx;
uniform int brush_value;
uniform int stage;
uniform int iteration;

shared float shared_data[1024];

void parallel_reduction(float result) {
    int globalID = int(gl_GlobalInvocationID.x);
    int localID = int(gl_LocalInvocationID.x);

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
    }
}

float Ad_i() {
    float Ad_i = 0.0;
    for (int i = 0; i < M; i++) {
        int idx = A[(N * M) + (globalID * M + i)];
        if (idx != -1) Ad_i += A[globalID * M + i] * d[idx];
    }
    return Ad_i;
}

void main() {
    int globalID = int(gl_GlobalInvocationID.x);
    int localID = int(gl_LocalInvocationID.x);

    if (globalID < N) {
        switch (stage) {
            case 0: { // Calculate dot(r_i, r_i), Store in r_i_norm (Only occurs on the first iteration of CGM)
                parallel_reduction(first_pass ? r[globalID] * r[globalID] : buffer_result[globalID]);
                r_i_norm = buffer_result[0];
                r_0_norm = r_i_norm;
            } break;
            case 1: { // Calculate dot(d_i, A * d_i), Store in d_iA_norm
                parallel_reduction(first_pass ? d[globalID] * Ad_i() : buffer_result[globalID]);
                d_iA_norm = buffer_result[0];
            } break;
            case 2: { // Update x and r
                float alpha = r_i_norm / d_iA_norm;

                x[globalID] = x[globalID] + alpha * d[globalID];
                r[globalID] = r[globalID] - alpha * Ad_i();
            } break;
            case 3: { // Calculate dot(r_(i+1), r_(i+1))
                parallel_reduction(first_pass ? r[globalID] * r[globalID] : buffer_result[globalID]);
                r_i1_norm = buffer_result[0];
            } break;
            case 4: { // Use the Gram-Schimdt constant to find the next search direction
                float gram_schimdt_constant = r_i1_norm / r_i_norm;            
                
                d[globalID] = r[globalID] + gram_schimdt_constant * d[globalID];

                r_i_norm = r_i1_norm;
            } break;
        }
    }
}