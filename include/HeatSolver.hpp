#pragma once
#include <Eigen/Sparse>

#include "Surface.hpp"

class HeatSolver {
public:
    std::shared_ptr<Surface> surface;
    float conductivity = 0.05f;
    float time_step = 0.01f;

    std::vector<int> idx_map;
    Eigen::SparseMatrix<float, Eigen::RowMajor> stiffness_matrix;
    Eigen::SparseMatrix<float, Eigen::RowMajor> mass_matrix;

    void init();
    void assemble_stiffness_matrix();
    void assemble_mass_matrix();
    void advance_time();
};