#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include "HeatSolver.hpp"

#include <iostream>

void HeatSolver::advance_time() {
    Eigen::VectorXf solution_vector = get_solution_vector();

    Eigen::VectorXf b = (mass_matrix / time_step) * solution_vector;
    Eigen::SparseMatrix<float> A = (mass_matrix / time_step) + (conductivity * stiffness_matrix);

    Eigen::ConjugateGradient<Eigen::SparseMatrix<float>, Eigen::Lower|Eigen::Upper> cg;
    cg.compute(A);
    solution_vector = cg.solve(b);

    map_solution_vector_to_surface(solution_vector);
}