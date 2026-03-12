#include <Eigen/Dense>

#include "FEM/FEMContext.hpp"

#include <iostream>

FEMContext::FEMContext() {
    parameters[Equation::Heat] = std::make_shared<HeatParameters>();
    parameters[Equation::Advection_Diffusion] = std::make_shared<AdvectionDiffusionParameters>();
    parameters[Equation::Wave] = std::make_shared<WaveParameters>();
    parameters[Equation::Reaction_Diffusion] = std::make_shared<ReactionDiffusionParameters>();
}

void FEMContext::init_from_surface(std::shared_ptr<Surface> surface) {
    if (surface->initialized) {
        this->surface = surface;
        num_elements = surface->triangles.size();
        update_boundary_conditions();
    }
}

unsigned int FEMContext::num_nodes() {
    return surface->vertices.size();
}

/**
 * Returns the total number of nodes that are considered unknown based on the boundary conditions
 * 
 * Dirichlet boundary conditions: (total nodes on mesh) - (nodes on boundary)
 * Since boundary nodes are fixed to 0, their values are known 
 * 
 * Neumann boundary conditions: (total nodes on mesh)
 * Since boundary nodes can take on any value, their values are unknown
 */
unsigned int FEMContext::num_unknowns() {
    return surface->vertices.size() - (static_cast<BoundaryCondition>(boundary_condition) == BoundaryCondition::Dirichlet ? surface->num_boundary_points : 0);
}

unsigned int FEMContext::num_max_nonzeros_per_row() {
    return this->max_row_nonzeros;
}

/**
 * Assemble all the matrices (stiffness, mass, and advection)
 */
void FEMContext::assemble_matrices() {
    Eigen::Vector3f velocity = std::static_pointer_cast<AdvectionDiffusionParameters>(parameters[Equation::Advection_Diffusion])->velocity;

    assemble_stiffness_matrix();
    assemble_mass_matrix();
    assemble_advection_matrix(velocity);
    this->max_row_nonzeros = compute_max_row_nonzeros();
}

/**
 * Assemble the stiffness matrix.
 * This matrix comprises integrals of ∇(phi_i) dot ∇(phi_j) over the problem domain, where phi is a linear basis function.
 */
void FEMContext::assemble_stiffness_matrix() {
    std::vector<Eigen::Triplet<float>> matrix_entries;
    for (int k = 0; k < num_elements; k++) {
        glm::vec3 a = surface->vertices[surface->triangles[k][0]];
        glm::vec3 b = surface->vertices[surface->triangles[k][1]];
        glm::vec3 c = surface->vertices[surface->triangles[k][2]];
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        float area = 0.5f * glm::length(glm::cross(b - a, c - a));

        std::array reference_gradients = {
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
                if (static_cast<BoundaryCondition>(boundary_condition) == BoundaryCondition::Neumann || !surface->on_boundary[surface->triangles[k][i]] && !surface->on_boundary[surface->triangles[k][j]]) {
                    matrix_entries.push_back(Eigen::Triplet<float>(
                        idx_map[surface->triangles[k][i]],
                        idx_map[surface->triangles[k][j]], 
                        area * physical_gradients[i].dot(physical_gradients[j])
                    ));
                }
            }
        }
    }

    stiffness_matrix.resize(num_unknowns(), num_unknowns());
    stiffness_matrix.setFromTriplets(matrix_entries.begin(), matrix_entries.end());
    stiffness_matrix.makeCompressed();
}

/**
 * Assemble the mass matrix.
 * This matrix comprises integrals of phi_i * phi_j over the problem domain, where phi is a linear basis function.
 */
void FEMContext::assemble_mass_matrix() {
    std::vector<Eigen::Triplet<float>> matrix_entries;
    for (int k = 0; k < num_elements; k++) {
        glm::vec3 a = surface->vertices[surface->triangles[k][0]];
        glm::vec3 b = surface->vertices[surface->triangles[k][1]];
        glm::vec3 c = surface->vertices[surface->triangles[k][2]];

        float jacobian_determinant = Eigen::Vector3<float>(b.x - a.x, b.y - a.y, b.z - a.z).cross(Eigen::Vector3<float>(c.x - a.x, c.y - a.y, c.z - a.z)).norm();
        Eigen::Matrix<float, 3, 3> local_mass_matrix {
            {2, 1, 1},
            {1, 2, 1},
            {1, 1, 2}
        };
        local_mass_matrix = (jacobian_determinant / 24.0) * local_mass_matrix;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (static_cast<BoundaryCondition>(boundary_condition) == BoundaryCondition::Neumann || !surface->on_boundary[surface->triangles[k][i]] && !surface->on_boundary[surface->triangles[k][j]]) {
                    matrix_entries.push_back(Eigen::Triplet<float>(
                        idx_map[surface->triangles[k][i]],
                        idx_map[surface->triangles[k][j]], 
                        local_mass_matrix(i, j)
                    ));
                }
            }
        }
    }

    mass_matrix = Eigen::SparseMatrix<float>(num_unknowns(), num_unknowns());
    mass_matrix.setFromTriplets(matrix_entries.begin(), matrix_entries.end());
    mass_matrix.makeCompressed();
}

/**
 * Assemble the advection matrix.
 * This matrix comprises integrals of phi_i * (velocity dot ∇(phi_i)) over the problem domain, where phi is a linear basis function.
 */
void FEMContext::assemble_advection_matrix(Eigen::Vector3f velocity) {
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

    advection_matrix = Eigen::SparseMatrix<float>(num_unknowns(), num_unknowns());
    advection_matrix.setFromTriplets(matrix_entries.begin(), matrix_entries.end());
    advection_matrix.makeCompressed();
}

int FEMContext::compute_max_row_nonzeros() {
    Eigen::SparseMatrix<float, Eigen::RowMajor> A = stiffness_matrix;

    int M = 0;
    for (int i = 0; i < A.outerSize(); i++) {
        int count = 0;
        for (typename Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(A, i); it; ++it)
            count++;
        M = std::max(M, count);
    }

    return M;
}

/**
 * Update the index map to reflect the surface's boundary conditions
 */
void FEMContext::update_boundary_conditions() {
    int idx = 0;
    idx_map = std::vector<int>(surface->vertices.size(), -1);
    for (int i = 0; i < surface->vertices.size(); i++) {
        switch (static_cast<BoundaryCondition>(boundary_condition)) {
            case BoundaryCondition::Dirichlet:
                if (!surface->on_boundary[i])
                    idx_map[i] = idx++;
                break;
            case BoundaryCondition::Neumann: 
                idx_map[i] = idx++;
                break;
        }
    }

    assemble_matrices();
}