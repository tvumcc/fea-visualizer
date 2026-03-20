#pragma once
#include "FEM/FEMContext.hpp"

/**
 * A base class that stores a pointer to a FEMContext object.
 * This class is inherited from by CPUSolver and GPUSolver.
 */
class Solver {
public:
    std::shared_ptr<FEMContext> fem_ctx;

    virtual void advance_time() = 0;
    virtual void clear_values() = 0;
    virtual bool has_numerical_instability() = 0;
};