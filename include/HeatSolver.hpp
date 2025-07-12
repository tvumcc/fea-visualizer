#pragma once
#include <Eigen/Sparse>

#include "Surface.hpp"
#include "Solver.hpp"

/**
 * Solver for the 2D Heat Equation:
 * u_t = a * u_xx
 */
class HeatSolver : public Solver {
public:
    float conductivity = 0.05f;
    float time_step = 0.01f;

    void advance_time() override;
};