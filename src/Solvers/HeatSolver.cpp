#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include "Solvers/HeatSolver.hpp"

#include <iostream>

void HeatSolver::clear_values() {
    u.resize(surface->num_unknown_nodes());
    u.setZero();
}

void HeatSolver::advance_time() {
    u = get_surface_value_vector();

    Eigen::SparseMatrix<float> A = (mass_matrix / time_step) + (conductivity * stiffness_matrix);
    Eigen::VectorXf b = (mass_matrix / time_step) * u;

    Eigen::ConjugateGradient<Eigen::SparseMatrix<float>, Eigen::Lower|Eigen::Upper> cg;
    cg.compute(A);
    u = cg.solve(b);

    map_vector_to_surface(u);
}

bool HeatSolver::has_numerical_instability() {
    for (int i = 0; i < u.size(); i++) {
        if (u.coeff(i) > 1e4f) {
            return true;
        }
    }
    return false;
}

void HeatSolver::init_gpu_solver() {
    unsigned int M = 0;
    Eigen::SparseMatrix<float, Eigen::RowMajor> A = (mass_matrix / time_step) + (conductivity * stiffness_matrix);

    std::vector<std::vector<float>> A_values;
    std::vector<std::vector<unsigned int>> A_indices;

    for (int i = 0; i < A.outerSize(); i++) {
        std::vector<float> values;
        std::vector<unsigned int> indices;

        for (typename Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(A, i); it; ++it) {
            values.push_back(it.value());
            indices.push_back(it.index());
        }

        M = std::max(M, static_cast<unsigned int>(values.size()));
        A_values.push_back(values);
        A_indices.push_back(indices);
    }

    gpu_cgm_solver = std::make_shared<GPUConjGrad>(surface->num_unknown_nodes(), M, surface->vertices.size(), idx_map);

    gpu_cgm_solver->loadSparseMatrix(gpu_cgm_solver->A1, A_values, A_indices);
}

void HeatSolver::advance_time_gpu() {
    // TODO: Load the known vector
    gpu_cgm_solver->solve(gpu_cgm_solver->A1, gpu_cgm_solver->x1, gpu_cgm_solver->known);
}