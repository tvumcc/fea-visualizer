#pragma once
#include <Eigen/Sparse>

#include "Surface.hpp"

/**
 * Base class for a Finite Element solver on a surface.
 */
class Solver {
public:
    std::shared_ptr<Surface> surface;

    std::vector<int> idx_map;
    Eigen::SparseMatrix<float> stiffness_matrix;
    Eigen::SparseMatrix<float> mass_matrix;

    void init();
    virtual void advance_time() = 0;
protected:
    void assemble_stiffness_matrix();
    void assemble_mass_matrix();
    virtual void assemble();
    Eigen::VectorXf get_surface_value_vector();
    void map_vector_to_surface(Eigen::VectorXf solution_vector);
};