#include "FEM/GPUSolver.hpp"

GPUSolver::GPUSolver(std::shared_ptr<FEMContext> fem_ctx) {
    this->fem_ctx = fem_ctx;

    init_buffers();
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

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->state);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, (fem_ctx->num_unknowns() + 7) * sizeof(float), NULL, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->known);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->residuals);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->search_directions);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->idx_map);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_nodes() * sizeof(unsigned int), fem_ctx->idx_map.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->u);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->v);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fem_ctx->num_unknowns() * sizeof(float), NULL, GL_STATIC_DRAW);
}

void GPUSolver::bind_buffers(unsigned int vector_buffer) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::State), this->state);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Known), this->known);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Residuals), this->residuals);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::SearchDirections), this->idx_map);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::SurfaceValues), fem_ctx->surface->get_value_buffer());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::StiffnessMatrix), this->stiffness_matrix);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::MassMatrix), this->mass_matrix);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::AdvectionMatrix), this->advection_matrix);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::MatrixIndices), this->matrix_indices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BindingPoint::Vector), vector_buffer);
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

    std::vector<unsigned int> indices_buffer = std::vector<unsigned int>(N * M, 0);
    std::vector<float> stiffness_buffer = std::vector<float>(N * M, 0);
    std::vector<float> mass_buffer = std::vector<float>(N * M, 0);
    std::vector<float> advection_buffer = std::vector<float>(N * M, 0);

    // TODO: Populate the buffers

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, matrix_indices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(unsigned int), indices_buffer.data(), GL_STATIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, stiffness_matrix);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), stiffness_buffer.data(), GL_STATIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mass_matrix);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), mass_buffer.data(), GL_STATIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, advection_matrix);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N * M * sizeof(float), advection_buffer.data(), GL_STATIC_READ);
}

void GPUSolver::advance_time() {
    switch (fem_ctx->equation) {
        case Equation::Heat: {

        } break;

        case Equation::Advection_Diffusion: {

        } break;

        case Equation::Wave: {

        } break;

        case Equation::Reaction_Diffusion: {

        } break;
    }
}

void GPUSolver::clear_values() {

}

bool GPUSolver::has_numerical_instability() {
    return false;
}