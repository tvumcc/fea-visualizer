#pragma once
#include "Solvers/Solver.hpp"

/**
 * Solver for the 2D Heat Equation:
 * https://en.wikipedia.org/wiki/Heat_equation
 */
class HeatSolver : public Solver {
public:
    Eigen::VectorXf u;
    
    float conductivity = 0.05f;
    float time_step = 0.01f;

    void clear_values() override;
    void advance_time() override;
    bool has_numerical_instability() override;
};