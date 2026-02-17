#pragma once
#include <Eigen/Sparse>

#include "Utils/Surface.hpp"
#include "Utils/Shader.hpp"

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
    void update_boundary_conditions();
    virtual void advance_time() = 0;
    virtual void clear_values() = 0;
    virtual bool has_numerical_instability() = 0;
    virtual void assemble();

    unsigned int buffer_input, buffer_result;
    std::shared_ptr<ComputeShader> compute_shader;

    void setup_dot_product_vectors(int N);
    void compute_dot_product(int N);
protected:
    void assemble_stiffness_matrix();
    void assemble_mass_matrix();
    Eigen::VectorXf get_surface_value_vector();
    void map_vector_to_surface(Eigen::VectorXf solution_vector);
};