#include <Eigen/Dense>
#include <Eigen/MatrixFunctions>

#include "HeatSolver.hpp"

void HeatSolver::init() {
    if (surface) {
        int idx = 0;
        idx_map = std::vector<int>(surface->vertices.size(), -1);
        for (int i = 0; i < surface->vertices.size(); i++)
            if (!surface->on_boundary[i])
                idx_map[i] = idx++;

        assemble_stiffness_matrix();
        assemble_mass_matrix();
    }
}

void HeatSolver::assemble_stiffness_matrix() {
    int num_elements = surface->triangles.size() / 3;
    int num_nodes = surface->vertices.size() - surface->num_boundary_points; // For Dirichlet BCs

    std::vector<Eigen::Triplet<float>> matrix_entries;
    for (int k = 0; k < num_elements; k++) {
        glm::vec3 a = surface->vertices[surface->triangles[k * 3 + 0]];
        glm::vec3 b = surface->vertices[surface->triangles[k * 3 + 1]];
        glm::vec3 c = surface->vertices[surface->triangles[k * 3 + 2]];

        std::array<Eigen::Vector2f, 3> reference_gradients = {
            Eigen::Vector2f {-1.0, -1.0},
            Eigen::Vector2f {1.0, 0.0},
            Eigen::Vector2f {0.0, 1.0},
        };

        Eigen::Matrix<float, 3, 2> A {
            {b.x - a.x, c.x - a.x},
            {b.y - a.y, c.y - a.y},
            {b.z - a.z, c.z - a.z}
        };
        A = A.inverse();

        std::array<Eigen::Vector2f, 3> physical_gradients;
        for (int i = 0; i < 3; i++) 
            physical_gradients[i] = reference_gradients[i] * A;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (!surface->on_boundary[surface->triangles[k * 3 + i]] && !surface->on_boundary[surface->triangles[k * 3 + j]]) {
                    matrix_entries.push_back(Eigen::Triplet<float>(
                        surface->triangles[k * 3 + i],
                        surface->triangles[k * 3 + j], 
                        physical_gradients[i].dot(physical_gradients[j])
                    ));
                }
            }
        }
    }

    stiffness_matrix = Eigen::SparseMatrix<float>(num_nodes, num_nodes);
    stiffness_matrix.setFromTriplets(matrix_entries.begin(), matrix_entries.end());
}

void HeatSolver::assemble_mass_matrix() {

}

void HeatSolver::advance_time() {

}