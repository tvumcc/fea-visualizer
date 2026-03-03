#version 460
layout (local_size_x = 1024) in;

layout (std430, binding = 0) buffer State {
    float r_i_norm;
    float r_i1_norm;
    float r_0_norm;
    float d_iA_norm;

    int N;
    int M;
    int total_nodes;
    float result[];
};

layout (std430, binding = 1) buffer Known {float b[];}; // Size of N
layout (std430, binding = 2) buffer Residuals {float r[];}; // Size of N
layout (std430, binding = 3) buffer SearchDirections {float d[];}; // Size of N
layout (std430, binding = 4) buffer IndexMap {int idx_map[];}; // Size of total_nodes (which equals N in the Neumann BC case)
layout (std430, binding = 5) buffer SurfaceValues {float values[];}; // Size of total_nodes (which equals N in the Neumann BC case)

layout (std430, binding = 6) buffer StiffnessMatrix {float stiffness[];};
layout (std430, binding = 7) buffer MassMatrix {float mass[];};
layout (std430, binding = 8) buffer AdvectionMatrix {float advection[];};
layout (std430, binding = 9) buffer MatrixIndices {int matrix_indices[];}; // Size of N*M; The indices in ELL format
layout (std430, binding = 10) buffer VectorU {float u[];}; // Size of N
layout (std430, binding = 11) buffer VectorV {float v[];}; // Size of N

uniform int stage;

uniform int solver;
uniform float time_step;
uniform float c;
uniform float Du;
uniform float Dv;
uniform float kill_rate;
uniform float feed_rate;

uniform int brush_idx;
uniform float brush_strength;

void main() {
    int globalID = int(gl_GlobalInvocationID.x);
    int localID = int(gl_LocalInvocationID.x);

    switch (stage) {
        case 0: { // Map surface to solution vector and process brush (# invocations = total_nodes)
            if (globalID == brush_idx) {
                values[globalID] = brush_strength;
            }

            if (idx_map[globalID] != -1) u[idx_map[globalID]] = values[globalID];
        } break;
        case 1: { // Populate the known vector, b (# invocations = N)
            float mvp = 0.0f;
            for (int i = 0; i < M; i++) {
                int mat_idx = globalID * M + i;
                int col_idx = matrix_indices[mat_idx];

                if (col_idx != -1) {
                    switch (solver) {
                        case 0: { // Heat Equation
                            mvp += u[col_idx] * (mass[mat_idx] / time_step);
                        } break;
                        case 1: { // Advection-Diffusion Equation
                            mvp += u[col_idx] * (mass[mat_idx] / time_step);
                        } break;
                        case 2: { // Wave Equation
                            mvp += v[col_idx] * (mass[mat_idx] / time_step) - c * c * u[col_idx] * stiffness[mat_idx]; 
                        } break;
                        case 3: { // Gray-Scott Reaction-Diffusion Equation (Step 1)
                            mvp += u[col_idx] * (mass[mat_idx] / time_step) - u[col_idx] * v[col_idx] * v[col_idx] + feed_rate * (1 - u[col_idx]);
                        } break;
                        case 4: { // Gray-Scott Reaction-Diffusion Equation (Step 2)
                            mvp += v[col_idx] * (mass[mat_idx] / time_step) + u[col_idx] * v[col_idx] * v[col_idx] - v[col_idx] * (feed_rate + kill_rate);
                        } break;
                    }
                }
            }
            b[globalID] = mvp;
        } break;
        case 2: { // Map solution vector to surface (# invocations = total_nodes)
            values[globalID] = idx_map[globalID] != -1 ? u[idx_map[globalID]] : 0.0f;
        } break;
    }

}