#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include "Solver.hpp"

/**
 * Initialize the solver only if the associated surface is initialized.
 */
void Solver::init() {
    if (surface) {
        int idx = 0;
        idx_map = std::vector<int>(surface->vertices.size(), -1);
        for (int i = 0; i < surface->vertices.size(); i++)
            if (!surface->on_boundary[i])
                idx_map[i] = idx++;

        assemble();
    }
}

/**
 * Assemble all of the matrices needed for solving.
 */
void Solver::assemble() {
    assemble_stiffness_matrix();
    assemble_mass_matrix();
}

/**
 * Assemble the stiffness matrix.
 * This matrix is comprised of integrals of ∂_x(phi_i) * ∂_x(phi_j) over the problem domain, where phi is a linear basis function.
 */
void Solver::assemble_stiffness_matrix() {
    int num_elements = surface->triangles.size() / 3;
    int num_nodes = surface->vertices.size() - surface->num_boundary_points; // For Dirichlet BCs

    std::vector<Eigen::Triplet<float>> matrix_entries;
    for (int k = 0; k < num_elements; k++) {
        glm::vec3 a = surface->vertices[surface->triangles[k * 3 + 0]];
        glm::vec3 b = surface->vertices[surface->triangles[k * 3 + 1]];
        glm::vec3 c = surface->vertices[surface->triangles[k * 3 + 2]];
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));

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

        std::array<Eigen::Vector3f, 3> physical_gradients;
        for (int i = 0; i < 3; i++)
            physical_gradients[i] = A * reference_gradients[i];

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (!surface->on_boundary[surface->triangles[k * 3 + i]] && !surface->on_boundary[surface->triangles[k * 3 + j]]) {
                    matrix_entries.push_back(Eigen::Triplet<float>(
                        idx_map[surface->triangles[k * 3 + i]],
                        idx_map[surface->triangles[k * 3 + j]], 
                        physical_gradients[i].dot(physical_gradients[j])
                    ));
                }
            }
        }
    }

    stiffness_matrix.resize(num_nodes, num_nodes);
    stiffness_matrix.setFromTriplets(matrix_entries.begin(), matrix_entries.end());
}

/**
 * Assemble the mass matrix.
 * This matrix is comprised of integrals of phi_i * phi_j over the problem domain, where phi is a linear basis function.
 */
void Solver::assemble_mass_matrix() {
    int num_elements = surface->triangles.size() / 3;
    int num_nodes = surface->vertices.size() - surface->num_boundary_points; // For Dirichlet BCs

    std::vector<Eigen::Triplet<float>> matrix_entries;
    for (int k = 0; k < num_elements; k++) {
        glm::vec3 a = surface->vertices[surface->triangles[k * 3 + 0]];
        glm::vec3 b = surface->vertices[surface->triangles[k * 3 + 1]];
        glm::vec3 c = surface->vertices[surface->triangles[k * 3 + 2]];

        float jacobian_determinant = Eigen::Vector3<float>(b.x - a.x, b.y - a.y, b.z - a.z).cross(Eigen::Vector3<float>(c.x - a.x, c.y - a.y, c.z - a.z)).norm();
        Eigen::Matrix<float, 3, 3> local_mass_matrix {
            {2, 1, 1},
            {1, 2, 1},
            {1, 1, 2}
        };
        local_mass_matrix = (jacobian_determinant / 24.0) * local_mass_matrix;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (!surface->on_boundary[surface->triangles[k * 3 + i]] && !surface->on_boundary[surface->triangles[k * 3 + j]]) {
                    matrix_entries.push_back(Eigen::Triplet<float>(
                        idx_map[surface->triangles[k * 3 + i]],
                        idx_map[surface->triangles[k * 3 + j]], 
                        local_mass_matrix(i, j)
                    ));
                }
            }
        }
    }

    mass_matrix = Eigen::SparseMatrix<float>(num_nodes, num_nodes);
    mass_matrix.setFromTriplets(matrix_entries.begin(), matrix_entries.end());
    mass_matrix.makeCompressed();
}

/**
 * Map values from the surface to an Eigen::VectorXf while ignoring the boundary nodes.
 */
Eigen::VectorXf Solver::get_solution_vector() {
    Eigen::VectorXf solution_vector;
    solution_vector.resize(surface->vertices.size() - surface->num_boundary_points);
    for (int i = 0; i < idx_map.size(); i++)
        if (idx_map[i] != -1)
            solution_vector.coeffRef(idx_map[i], 0) = surface->values[i];

    return solution_vector;
}

/**
 * Map values from an Eigen::VectorXf back onto the surface while ignoring the boundary nodes.
 */
void Solver::map_solution_vector_to_surface(Eigen::VectorXf solution_vector) {
    for (int i = 0; i < idx_map.size(); i++) {
        if (idx_map[i] != -1) {
            surface->values[i] = solution_vector.coeff(idx_map[i], 0);
        } else {
            surface->values[i] = 0.0f;
        }
    }
}