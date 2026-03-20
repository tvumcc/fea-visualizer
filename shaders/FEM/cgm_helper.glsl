/*
    cgm_helper.glsl

    This compute shader performs helper procedures for the
    Conjugate Gradient Method such as mapping solution vectors to a surface,
    loading vectors, and intermediate steps for certain equations.
*/

#version 460
layout (local_size_x = 1024) in;

layout (std430, binding = 0) buffer State {
    float r_0_norm;
    float r_i_norm;
    float r_i1_norm;
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

uniform int equation;
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
            if (globalID < total_nodes) {
                if (globalID == brush_idx) {
                    values[globalID] = brush_strength;
                }

                switch (equation) {
                    case 0: // Heat Equation
                    case 1: // Advection-Diffusion Equation
                    case 2: // Wave Equation
                        if (idx_map[globalID] != -1) u[idx_map[globalID]] = values[globalID];
                        break;
                    case 3: // Gray-Scott Reaction-Diffusion Equation (Step 1)
                    case 4: // Gray-Scott Reaction-Diffusion Equation (Step 2)
                        if (idx_map[globalID] != -1) v[idx_map[globalID]] = values[globalID];
                        break;
                }
            }
        } break;
        case 1: { // Initialize vectors (# invocations = N)
            if (globalID < N) {
                float b_i = 0.0;
                float Ax_i = 0.0;
                for (int i = 0; i < M; i++) {
                    int mat_idx = globalID * M + i;
                    int col_idx = matrix_indices[mat_idx];

                    if (col_idx != -1) {
                        switch (equation) {
                            case 0: { // Heat Equation
                                b_i += u[col_idx] * (mass[mat_idx] / time_step);
                                Ax_i += u[col_idx] * (mass[mat_idx] / time_step + c * stiffness[mat_idx]);
                            } break;
                            case 1: { // Advection-Diffusion Equation
                                b_i += u[col_idx] * (mass[mat_idx] / time_step);
                                Ax_i += u[col_idx] * (mass[mat_idx] / time_step + c * stiffness[mat_idx] - advection[mat_idx]);
                            } break;
                            case 2: { // Wave Equation
                                b_i += v[col_idx] * (mass[mat_idx] / time_step) - c * c * u[col_idx] * stiffness[mat_idx]; 
                                Ax_i += v[col_idx] * (mass[mat_idx] / time_step + c * c * stiffness[mat_idx] * time_step);
                            } break;
                            case 3: { // Gray-Scott Reaction-Diffusion Equation (Step 1)
                                b_i += u[col_idx] * (mass[mat_idx] / time_step) - u[col_idx] * v[col_idx] * v[col_idx] + feed_rate * (1.0 - u[col_idx]);
                                Ax_i += u[col_idx] * (mass[mat_idx] / time_step + Du * stiffness[mat_idx]);
                            } break;
                            case 4: { // Gray-Scott Reaction-Diffusion Equation (Step 2)
                                b_i += v[col_idx] * (mass[mat_idx] / time_step) + u[col_idx] * v[col_idx] * v[col_idx] - v[col_idx] * (feed_rate + kill_rate);
                                Ax_i += v[col_idx] * (mass[mat_idx] / time_step + Dv * stiffness[mat_idx]);
                            } break;
                        }
                    }
                }

                b[globalID] = b_i;
                d[globalID] = b_i - Ax_i;
                r[globalID] = b_i - Ax_i;
                result[globalID] = 0.0;
                r_0_norm = 0.0;
                r_i_norm = 0.0;
                r_i1_norm = 0.0;
                d_iA_norm = 0.0;
            }
        } break;
        case 2: { // Wave Equation update (# invocations = N)
            switch (equation) {
                case 2: { // Wave Equation
                    if (globalID < N) {
                        u[globalID] = u[globalID] + v[globalID] * time_step;
                    }
                } break;
            }
        } break;
        case 3: { // Map solution vector to surface (# invocations = total_nodes)
            if (globalID < total_nodes) {
                switch (equation) {
                    case 0: // Heat Equation
                    case 1: // Advection-Diffusion Equation
                    case 2: // Wave Equation
                        values[globalID] = idx_map[globalID] != -1 ? u[idx_map[globalID]] : 0.0;
                        break;
                    case 3: // Gray-Scott Reaction-Diffusion Equation (Step 1)
                    case 4: // Gray-Scott Reaction-Diffusion Equation (Step 2)
                        values[globalID] = v[idx_map[globalID]];
                        break;
                }

                if (globalID == brush_idx) {
                    values[globalID] = brush_strength;
                }
            }
        } break;
        case 4: { // Reset all nodal values to 0
            if (globalID < N) {
                values[globalID] = 0.0;
            }
        } break;
    }
}