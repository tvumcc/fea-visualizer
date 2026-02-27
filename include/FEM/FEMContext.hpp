#pragma once
#include <Eigen/Sparse>

#include "Utils/Surface.hpp"
#include "FEM/EquationParameters.hpp"

#include <memory>

enum class Equation {
    Heat = 0,
    Wave,
    Advection_Diffusion,
    Reaction_Diffusion,
};

enum class BoundaryCondition {
    Dirichlet = 0,
    Neumann, 
};

class FEMContext {
public:
    std::shared_ptr<Surface> surface;
    std::unordered_map<Equation, std::shared_ptr<EquationParameters>> parameters;
    std::vector<int> idx_map;

    Equation equation = Equation::Wave;
    BoundaryCondition boundary_condition = BoundaryCondition::Dirichlet;

    FEMContext();

    void init_from_surface(std::shared_ptr<Surface> surface);
    void update_boundary_conditions();
    void assemble_matrices();

    unsigned int num_nodes();
    unsigned int num_unknowns();
    unsigned int num_max_nonzeros_per_row();
private:
    int num_elements;
    int max_row_nonzeros;

    Eigen::SparseMatrix<float> stiffness_matrix;
    Eigen::SparseMatrix<float> mass_matrix;
    Eigen::SparseMatrix<float> advection_matrix;

    void assemble_stiffness_matrix();
    void assemble_mass_matrix();
    void assemble_advection_matrix(Eigen::Vector3f velocity);
    int compute_max_row_nonzeros();


    friend class CPUSolver;
    friend class GPUSolver;
};