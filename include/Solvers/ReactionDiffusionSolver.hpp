#pragma once
#include "Solvers/Solver.hpp"

/**
 * Solver for the 2D Gray-Scott Reaction-Diffusion Equation:
 * https://groups.csail.mit.edu/mac/projects/amorphous/GrayScott/ 
 */
class ReactionDiffusionSolver : public Solver {
public:
    Eigen::VectorXf u;
    Eigen::VectorXf v;
    Eigen::VectorXf load_vector;
    
    float Du = 0.08f;
    float Dv = 0.04f;
    float feed_rate = 0.035f;
    float kill_rate = 0.06f;

    float time_step = 0.001f;

    void clear_values() override;
    void assemble() override;
    void advance_time() override;
    bool has_numerical_instability() override;
};