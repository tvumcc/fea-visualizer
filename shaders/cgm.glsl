#version 460
layout (local_size_x = 1024) in;

layout (std430, binding = 0) buffer State {
    float r_i_norm;
    float r_i1_norm;
    float r_0_norm;
    float d_iA_norm;

    uint N;
    uint M;
    uint total_nodes;
    float result[];
};

layout (std430, binding = 1) buffer Known {float b[];}; // Size of N
layout (std430, binding = 2) buffer Residuals {float r[];}; // Size of N
layout (std430, binding = 3) buffer SearchDirections {float d[];}; // Size of N
layout (std430, binding = 4) buffer IndexMap {uint idx_map[];}; // Size of total_nodes (which equals N in the Neumann BC case)
layout (std430, binding = 5) buffer SurfaceValues {float values[];}; // Size of total_nodes (which equals N in the Neumann BC case)

layout (std430, binding = 6) buffer StiffnessMatrix {float stiffness[];};
layout (std430, binding = 7) buffer MassMatrix {float mass[];};
layout (std430, binding = 8) buffer AdvectionMatrix {float advection[];};
layout (std430, binding = 9) buffer MatrixIndices {uint matrix_indices[];}; // Size of N*M; The indices in ELL format
layout (std430, binding = 10) buffer Vector {float x[];}; // Size of N

uniform bool first_pass;
uniform int stage;

uniform int solver;
uniform float time_step;
uniform float c;
uniform float Du;
uniform float Dv;
uniform float kill_rate;
uniform float feed_rate;

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
        result[gl_WorkGroupID.x] = shared_data[localID];
    }
}

float Ad_i() {
    float Ad_i = 0.0;
    for (int i = 0; i < M; i++) {
        int mat_idx = globalID * M + i;
        int col_idx = indices[mat_idx];

        if (col_idx != -1) {
            switch (solver) {
                case 0: // Heat Equation
                    Ad_i += d[col_idx] * ((mass[mat_idx] / time_step) + c * stiffness[mat_idx]);
                    break;
                case 1: // Advection-Diffusion Equation
                    Ad_i += d[col_idx] * ((mass[mat_idx] / time_step) + (c * stiffness[mat_idx]) - advection[mat_idx]);
                    break;
                case 2: // Wave Equation
                    Ad_i += d[col_idx] * ((mass[mat_idx] / time_step) + (c * c * stiffness[mat_idx] * time_step));
                    break;
                case 3: // Gray-Scott Reaction-Diffusion Equation (Step 1)
                    Ad_i += d[col_idx] * ((mass[mat_idx] / time_step) + (Du * stiffness[mat_idx]));
                    break;
                case 4: // Gray-Scott Reaction-Diffusion Equation (Step 2)
                    Ad_i += d[col_idx] * ((mass[mat_idx] / time_step) + (Dv * stiffness[mat_idx]));
                    break;
            }
        }
    }
    return Ad_i;
}

void main() {
    int globalID = int(gl_GlobalInvocationID.x);
    int localID = int(gl_LocalInvocationID.x);

    if (globalID < N) {
        switch (stage) {
            case 0: { // Calculate dot(r_i, r_i), Store in r_i_norm (Only occurs on the first iteration of CGM)
                parallel_reduction(first_pass ? r[globalID] * r[globalID] : result[globalID]);
                r_i_norm = result[0];
                r_0_norm = r_i_norm;
            } break;
            case 1: { // Calculate dot(d_i, A * d_i), Store in d_iA_norm
                parallel_reduction(first_pass ? d[globalID] * Ad_i() : result[globalID]);
                d_iA_norm = result[0];
            } break;
            case 2: { // Update x and r
                float alpha = r_i_norm / d_iA_norm;

                x[globalID] = x[globalID] + alpha * d[globalID];
                r[globalID] = r[globalID] - alpha * Ad_i();
            } break;
            case 3: { // Calculate dot(r_(i+1), r_(i+1))
                parallel_reduction(first_pass ? r[globalID] * r[globalID] : result[globalID]);
                r_i1_norm = result[0];
            } break;
            case 4: { // Use the Gram-Schimdt constant to find the next search direction
                float gram_schimdt_constant = r_i1_norm / r_i_norm;            
                
                d[globalID] = r[globalID] + gram_schimdt_constant * d[globalID];

                r_i_norm = r_i1_norm;
            } break;
        }
    }
}