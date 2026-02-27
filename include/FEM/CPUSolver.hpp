#pragma once

#include "FEM/Solver.hpp"
#include "FEM/FEMContext.hpp"

class CPUSolver : public Solver {
public:
    CPUSolver(std::shared_ptr<FEMContext> fem_ctx);

    void advance_time() override;
    void clear_values() override;
    bool has_numerical_instability() override;
private:
    Eigen::VectorXf u;
    Eigen::VectorXf v;

    Eigen::VectorXf get_surface_value_vector();
    void map_vector_to_surface(const Eigen::VectorXf& vector);
};