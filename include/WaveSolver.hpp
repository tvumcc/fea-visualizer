#pragma once
#include "Surface.hpp"
#include "Solver.hpp"

/**
 * Solver for the 2D Wave Equation:
 * https://en.wikipedia.org/wiki/Wave_equation
 */
class WaveSolver : public Solver {
public:
    Eigen::VectorXf u;
    Eigen::VectorXf v;
    
    float c = 0.05f;
    float time_step = 0.05f;

    void assemble() override;
    void advance_time() override;
};