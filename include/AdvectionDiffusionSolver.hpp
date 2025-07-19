#pragma once
#include "Surface.hpp"
#include "Solver.hpp"

/**
 * Solver for the Advection-Diffusion Equation:
 * https://en.wikipedia.org/wiki/Convection%E2%80%93diffusion_equation 
 */
class AdvectionDiffusionSolver : public Solver {
public:
    Eigen::VectorXf u;
    Eigen::SparseMatrix<float> advection_matrix;
    
    float c = 0.25f;
    Eigen::Vector3f velocity = {1.0f, 0.0f, 0.0f};
    float time_step = 0.001f;

    void assemble() override;
    void clear_values() override;
    void advance_time() override;
    bool has_numerical_instability() override;
private:
    void assemble_advection_matrix();
};