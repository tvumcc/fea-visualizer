#pragma once

#include "FEM/Solver.hpp"
#include "FEM/FEMContext.hpp"

enum class BindingPoint {
    State = 0,
    Known,
    Residuals,
    SearchDirections,
    IndexMap,
    SurfaceValues,

    StiffnessMatrix,
    MassMatrix,
    AdvectionMatrix,
    MatrixIndices,
    VectorU,
    VectorV,
};

class GPUSolver : public Solver {
public:
    std::shared_ptr<ComputeShader> cgm_compute_shader;
    std::shared_ptr<ComputeShader> cgm_helper_compute_shader;

    GPUSolver(std::shared_ptr<FEMContext> fem_ctx);
    ~GPUSolver();

    void advance_time() override;
    void clear_values() override;
    bool has_numerical_instability() override;

    void init();
    void brush(int brush_idx, float brush_strength);
private:
    unsigned int state;
    unsigned int known;
    unsigned int residuals;
    unsigned int search_directions;
    unsigned int idx_map;

    unsigned int matrix_indices;
    unsigned int stiffness_matrix;
    unsigned int mass_matrix;
    unsigned int advection_matrix;

    unsigned int u;
    unsigned int v;

    float* residual_norm_map;

    void init_buffers();
    void bind_buffers();

    void load_state();
    void load_matrices();

    void dot_product(int stage);
    void cgm_setup();
    void cgm();
    void cgm_cleanup();
};