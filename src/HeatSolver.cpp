#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include "HeatSolver.hpp"

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