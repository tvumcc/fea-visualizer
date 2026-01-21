#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include "Solvers/ReactionDiffusionSolver.hpp"

void ReactionDiffusionSolver::clear_values() {
    u.resize(surface->num_unknown_nodes());
    u.setZero();
    v.resize(surface->num_unknown_nodes());
    v.setZero();
    map_vector_to_surface(v);
}

void ReactionDiffusionSolver::assemble() {
    assemble_stiffness_matrix();
    assemble_mass_matrix();
}

void ReactionDiffusionSolver::advance_time() {
    v = get_surface_value_vector();

    Eigen::ConjugateGradient<Eigen::SparseMatrix<float>, Eigen::Lower|Eigen::Upper> cg;

    // Setup for a semi-implicit time stepping scheme
    Eigen::SparseMatrix<float> A_u = mass_matrix / time_step + Du * stiffness_matrix;
    Eigen::VectorXf b_u = (mass_matrix / time_step) * u - (u.cwiseProduct(v.cwiseProduct(v))) + feed_rate * (Eigen::VectorXf::Ones(surface->num_unknown_nodes()) - u);

    Eigen::SparseMatrix<float> A_v = mass_matrix / time_step + Dv * stiffness_matrix;
    Eigen::VectorXf b_v = (mass_matrix / time_step) * v + (u.cwiseProduct(v.cwiseProduct(v))) - (feed_rate + kill_rate) * v;

    cg.compute(A_u);
    u = cg.solve(b_u);
    cg.compute(A_v);
    v = cg.solve(b_v);

    map_vector_to_surface(v);
}

bool ReactionDiffusionSolver::has_numerical_instability() {
    for (int i = 0; i < u.size(); i++) {
        if (u.coeff(i) > 1e4f || v.coeff(i) > 1e4f) {
            return true;
        }
    }
    return false;
}