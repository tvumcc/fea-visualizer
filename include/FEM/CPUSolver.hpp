#pragma once

#include "FEM/Solver.hpp"
#include "FEM/FEMContext.hpp"

/**
 * A solver for finite element systems that uses Eigen, on the CPU
 */
class CPUSolver : public Solver {
public:
    CPUSolver(std::shared_ptr<FEMContext> fem_ctx);

    bool has_numerical_instability() override;
    void clear_values() override;
    void advance_time() override;
private:
    Eigen::VectorXf u;
    Eigen::VectorXf v;

    Eigen::VectorXf get_surface_value_vector();
    void map_vector_to_surface(const Eigen::VectorXf& vector);
};