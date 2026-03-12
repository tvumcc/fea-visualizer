#include "FEM/GPUSolver.hpp"

#include <iostream>

#define r_i_norm residual_norm_map[1]

GPUSolver::GPUSolver(std::shared_ptr<FEMContext> fem_ctx) {
    this->fem_ctx = fem_ctx;
}

GPUSolver::~GPUSolver() {
    glDeleteBuffers(1, &this->state);
    glDeleteBuffers(1, &this->known);
    glDeleteBuffers(1, &this->residuals);
    glDeleteBuffers(1, &this->search_directions);
    glDeleteBuffers(1, &this->idx_map);

    glDeleteBuffers(1, &this->matrix_indices);
    glDeleteBuffers(1, &this->stiffness_matrix);
    glDeleteBuffers(1, &this->mass_matrix);
    glDeleteBuffers(1, &this->advection_matrix);

    glDeleteBuffers(1, &this->u);
    glDeleteBuffers(1, &this->v);
}

void GPUSolver::init() {
    init_buffers();
    load_matrices();
    load_state();
    bind_buffers();
}

void GPUSolver::brush(int brush_idx, float brush_strength) {
    cgm_helper_compute_shader->bind();
    cgm_helper_compute_shader->set_int("brush_idx", brush_idx);
    cgm_helper_compute_shader->set_float("brush_strength", brush_strength);

    cgm_helper_compute_shader->set_int("stage", 0);
    cgm_helper_compute_shader->dispatch_compute(fem_ctx->num_nodes() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPUSolver::init_buffers() {
    glGenBuffers(1, &this->state);
    glGenBuffers(1, &this->known);
    glGenBuffers(1, &this->residuals);
    glGenBuffers(1, &this->search_directions);
    glGenBuffers(1, &this->idx_map);

    glGenBuffers(1, &this->matrix_indices);
    glGenBuffers(1, &this->stiffness_matrix);
    glGenBuffers(1, &this->mass_matrix);
    glGenBuffers(1, &this->advection_matrix);

    glGenBuffers(1, &this->u);
    glGenBuffers(1, &this->v);

    std::vector<float> zeros = std::vector<float>(fem_ctx->num_unknowns() + 7, 0.0f);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->state);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, (fem_ctx->num_unknowns() + 7) * sizeof(float), zeros.data(), GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    residual_norm_map = static_cast<float*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8 * sizeof(float), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->known);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), zeros.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->residuals);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), zeros.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->search_directions);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), zeros.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->idx_map);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_nodes() * sizeof(unsigned int), fem_ctx->idx_map.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->u);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), zeros.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->v);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), zeros.data(), GL_STATIC_DRAW);
}

void GPUSolver::bind_buffers() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::State), this->state);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Known), this->known);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Residuals), this->residuals);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::SearchDirections), this->search_directions);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::IndexMap), this->idx_map);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::SurfaceValues), fem_ctx->surface->get_value_buffer());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::StiffnessMatrix), this->stiffness_matrix);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::MassMatrix), this->mass_matrix);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::AdvectionMatrix), this->advection_matrix);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::MatrixIndices), this->matrix_indices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::VectorU), this->u);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::VectorV), this->v);
}

void GPUSolver::load_state() {
    unsigned int data[] = {
        fem_ctx->num_unknowns(), // N
        fem_ctx->num_max_nonzeros_per_row(), // M
        fem_ctx->num_nodes() // total_nodes
    };

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->state);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(float), 3 * sizeof(unsigned int), data);
}

void GPUSolver::load_matrices() {
    unsigned int N = fem_ctx->num_unknowns();
    unsigned int M = fem_ctx->num_max_nonzeros_per_row();

    std::vector<unsigned int> indices_buffer = std::vector<unsigned int>(N * M, -1);
    std::vector<float> stiffness_buffer = std::vector<float>(N * M, 0.0f);
    std::vector<float> mass_buffer = std::vector<float>(N * M, 0.0f);
    std::vector<float> advection_buffer = std::vector<float>(N * M, 0.0f);

    // TODO: Populate the buffers
    Eigen::SparseMatrix<float, Eigen::RowMajor> A = fem_ctx->stiffness_matrix;

    for (int i = 0; i < A.outerSize(); i++) {
        int j = 0; 
        for (Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(A, i); it; ++it, j++) {
            unsigned int row = i;
            unsigned int col = it.index();
            unsigned int buffer_idx = M * i + j;

            indices_buffer[buffer_idx] = col;
            stiffness_buffer[buffer_idx] = fem_ctx->stiffness_matrix.coeff(row, col);
            mass_buffer[buffer_idx] = fem_ctx->mass_matrix.coeff(row, col);
            advection_buffer[buffer_idx] = fem_ctx->advection_matrix.coeff(row, col);
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, matrix_indices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(unsigned int), indices_buffer.data(), GL_STATIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, stiffness_matrix);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), stiffness_buffer.data(), GL_STATIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mass_matrix);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), mass_buffer.data(), GL_STATIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, advection_matrix);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), advection_buffer.data(), GL_STATIC_READ);
}

void GPUSolver::dot_product(int stage) {
    bind_buffers();
    int work_group_size = 1024;
    int current_size = fem_ctx->num_unknowns();
    cgm_compute_shader->bind();
    cgm_compute_shader->set_int("stage", stage);
    cgm_compute_shader->set_bool("first_pass", true);

    while (current_size > 1) {
        // This math is to ensure that there are enough work groups
        int num_work_groups = (current_size + (work_group_size - 1)) / work_group_size;
        cgm_compute_shader->dispatch_compute(num_work_groups, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

        cgm_compute_shader->set_bool("first_pass", false);
        current_size = num_work_groups;
    }
}

void GPUSolver::cgm_setup() {
    cgm_helper_compute_shader->bind();

    // Map surface to solution vector and process brush (# invocations = total_nodes)
    cgm_helper_compute_shader->set_int("stage", 0);
    cgm_helper_compute_shader->dispatch_compute(fem_ctx->num_nodes() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);

    // Initialize vectors (# invocations = N)
    cgm_helper_compute_shader->set_int("stage", 1);
    cgm_helper_compute_shader->dispatch_compute(fem_ctx->num_unknowns() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPUSolver::cgm() {
    cgm_compute_shader->bind();

    // Stage 0: Calculate dot(r_i, r_i), Store in r_i_norm (Only occurs on the first iteration of CGM)
    dot_product(0);

    glFinish();
    if (r_i_norm > 1e-8) {
        // Stage 1: Calculate dot(d_i, A * d_i), Store in d_iA_norm
        dot_product(1);

        // Stage 2: Update u and r
        cgm_compute_shader->set_int("stage", 2);
        cgm_compute_shader->dispatch_compute(fem_ctx->num_unknowns() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
    }
}

void GPUSolver::cgm_cleanup() {
    // Map solution vector to surface (# invocations = total_nodes)
    cgm_helper_compute_shader->bind();
    cgm_helper_compute_shader->set_int("stage", 3);
    cgm_helper_compute_shader->dispatch_compute(fem_ctx->num_nodes() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPUSolver::advance_time() {
    bind_buffers();

    switch (fem_ctx->equation) {
        case Equation::Heat: {
            auto params = std::static_pointer_cast<HeatParameters>(fem_ctx->parameters[Equation::Heat]);

            cgm_helper_compute_shader->bind();
            cgm_helper_compute_shader->set_int("equation", 0);
            cgm_helper_compute_shader->set_float("time_step", params->time_step);
            cgm_helper_compute_shader->set_float("c", params->conductivity);
            
            cgm_compute_shader->bind();
            cgm_compute_shader->set_int("equation", 0); 
            cgm_compute_shader->set_float("time_step", params->time_step);
            cgm_compute_shader->set_float("c", params->conductivity);

            cgm_setup();
            cgm();
            cgm_cleanup();
        } break;

        case Equation::Advection_Diffusion: {
            auto params = std::static_pointer_cast<AdvectionDiffusionParameters>(fem_ctx->parameters[Equation::Advection_Diffusion]);

            cgm_helper_compute_shader->bind();
            cgm_helper_compute_shader->set_int("equation", 1);
            cgm_helper_compute_shader->set_float("time_step", params->time_step);
            cgm_helper_compute_shader->set_float("c", params->c);
            
            cgm_compute_shader->bind();
            cgm_compute_shader->set_int("equation", 1); 
            cgm_compute_shader->set_float("time_step", params->time_step);
            cgm_compute_shader->set_float("c", params->c);

            cgm_setup();
            cgm();
            cgm_cleanup();
        } break;

        case Equation::Wave: {
            auto params = std::static_pointer_cast<WaveParameters>(fem_ctx->parameters[Equation::Wave]);

            cgm_helper_compute_shader->bind();
            cgm_helper_compute_shader->set_int("equation", 2);
            cgm_helper_compute_shader->set_float("time_step", params->time_step);
            cgm_helper_compute_shader->set_float("c", params->c);

            cgm_compute_shader->bind();
            cgm_compute_shader->set_int("equation", 2); 
            cgm_compute_shader->set_float("time_step", params->time_step);
            cgm_compute_shader->set_float("c", params->c);

            cgm_setup();
            cgm();

            // Wave Equation Exclusive Step
            cgm_helper_compute_shader->bind();
            cgm_helper_compute_shader->set_int("stage", 2);
            cgm_helper_compute_shader->dispatch_compute(fem_ctx->num_unknowns() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);

            cgm_cleanup();
        } break;

        case Equation::Reaction_Diffusion: {
            auto params = std::static_pointer_cast<ReactionDiffusionParameters>(fem_ctx->parameters[Equation::Reaction_Diffusion]);

            cgm_helper_compute_shader->bind();
            cgm_helper_compute_shader->set_int("equation", 3);
            cgm_helper_compute_shader->set_float("time_step", params->time_step);
            cgm_helper_compute_shader->set_float("Du", params->Du);
            cgm_helper_compute_shader->set_float("Dv", params->Dv);
            cgm_helper_compute_shader->set_float("feed_rate", params->feed_rate);
            cgm_helper_compute_shader->set_float("kill_rate", params->kill_rate);

            cgm_compute_shader->bind();
            cgm_compute_shader->set_int("equation", 3);
            cgm_compute_shader->set_float("time_step", params->time_step);
            cgm_compute_shader->set_float("Du", params->Du);
            cgm_compute_shader->set_float("Dv", params->Dv);
            cgm_compute_shader->set_float("feed_rate", params->feed_rate);
            cgm_compute_shader->set_float("kill_rate", params->kill_rate);

            cgm_setup();
            cgm();

            cgm_helper_compute_shader->bind(); cgm_helper_compute_shader->set_int("equation", 4);
            cgm_compute_shader->bind(); cgm_compute_shader->set_int("equation", 4);
            
            // Initialize vectors (# invocations = N)
            cgm_helper_compute_shader->bind();
            cgm_helper_compute_shader->set_int("stage", 1);
            cgm_helper_compute_shader->dispatch_compute(fem_ctx->num_unknowns() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);

            cgm();
            cgm_cleanup();
        } break;
    }
}

void GPUSolver::clear_values() {
    cgm_helper_compute_shader->bind();
    cgm_helper_compute_shader->set_int("stage", 4);
    cgm_helper_compute_shader->dispatch_compute(fem_ctx->num_nodes() / 1024 + 1, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

bool GPUSolver::has_numerical_instability() {
    glFinish();
    return r_i_norm > 1e4;
}