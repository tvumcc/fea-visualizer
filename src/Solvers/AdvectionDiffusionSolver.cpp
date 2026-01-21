#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include "Solvers/AdvectionDiffusionSolver.hpp"

void AdvectionDiffusionSolver::assemble() {
    assemble_stiffness_matrix();
    assemble_mass_matrix();
    assemble_advection_matrix();
}

void AdvectionDiffusionSolver::clear_values() {
    u.resize(surface->num_unknown_nodes());
    u.setZero();
}

void AdvectionDiffusionSolver::assemble_advection_matrix() {
    int num_elements = surface->triangles.size();
    int num_nodes = surface->num_unknown_nodes(); // For Dirichlet BCs

    std::vector<Eigen::Triplet<float>> matrix_entries;
    for (int k = 0; k < num_elements; k++) {
        glm::vec3 a = surface->vertices[surface->triangles[k][0]];
        glm::vec3 b = surface->vertices[surface->triangles[k][1]];
        glm::vec3 c = surface->vertices[surface->triangles[k][2]];
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        float jacobian_determinant = Eigen::Vector3<float>(b.x - a.x, b.y - a.y, b.z - a.z).cross(Eigen::Vector3<float>(c.x - a.x, c.y - a.y, c.z - a.z)).norm();

        std::array<Eigen::Vector3f, 3> reference_gradients = {
            Eigen::Vector3f {-1.0, -1.0, 0.0},
            Eigen::Vector3f {1.0, 0.0, 0.0},
            Eigen::Vector3f {0.0, 1.0, 0.0},
        };

        Eigen::Matrix<float, 3, 3> A {
            {b.x - a.x, c.x - a.x, normal.x},
            {b.y - a.y, c.y - a.y, normal.y},
            {b.z - a.z, c.z - a.z, normal.z}
        };
        A = A.inverse().eval();
        A.transposeInPlace();

        Eigen::Vector3f plane_normal = {normal.x, normal.y, normal.z};
        Eigen::Vector3f transformed_velocity = (velocity - velocity.dot(plane_normal) / (plane_normal.norm() * plane_normal.norm()) * plane_normal).normalized();

        std::array<Eigen::Vector3f, 3> physical_gradients;
        for (int i = 0; i < 3; i++)
            physical_gradients[i] = A * reference_gradients[i];

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (!surface->on_boundary[surface->triangles[k][i]] && !surface->on_boundary[surface->triangles[k][j]]) {
                    matrix_entries.push_back(Eigen::Triplet<float>(
                        idx_map[surface->triangles[k][i]],
                        idx_map[surface->triangles[k][j]], 
                        (jacobian_determinant / 6.0f) * transformed_velocity.dot(physical_gradients[j])
                    ));
                }
            }
        }
    }

    advection_matrix = Eigen::SparseMatrix<float>(num_nodes, num_nodes);
    advection_matrix.setFromTriplets(matrix_entries.begin(), matrix_entries.end());
    advection_matrix.makeCompressed();
}

void AdvectionDiffusionSolver::advance_time() {
    u = get_surface_value_vector();

    Eigen::SparseMatrix<float> A = (mass_matrix / time_step) + (c * stiffness_matrix) - advection_matrix;
    Eigen::VectorXf b = (mass_matrix / time_step) * u;

    Eigen::ConjugateGradient<Eigen::SparseMatrix<float>, Eigen::Lower|Eigen::Upper> cg;
    cg.compute(A);
    u = cg.solve(b);

    map_vector_to_surface(u);
}

bool AdvectionDiffusionSolver::has_numerical_instability() {
    for (int i = 0; i < u.size(); i++) {
        if (u.coeff(i) > 1e4) {
            return true;
        }
    }
    return false;
}