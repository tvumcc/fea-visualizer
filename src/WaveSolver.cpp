#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include "WaveSolver.hpp"

#include <iostream>

void WaveSolver::clear_values() {
    u.resize(surface->num_unknown_nodes());
    u.setZero();
    v.resize(surface->num_unknown_nodes());
    v.setZero();
}

void WaveSolver::advance_time() {
    u = get_surface_value_vector();

    Eigen::ConjugateGradient<Eigen::SparseMatrix<float>, Eigen::Lower|Eigen::Upper> cg;
    Eigen::SparseMatrix<float> A_v = (mass_matrix / time_step) + (c * c * stiffness_matrix * time_step);
    Eigen::VectorXf b_v = (mass_matrix / time_step) * v - c * c * stiffness_matrix * u;

    cg.compute(A_v);
    v = cg.solve(b_v);
    u = u + v * time_step;

    map_vector_to_surface(u);
}