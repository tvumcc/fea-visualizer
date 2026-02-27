#include "FEM/CPUSolver.hpp"

CPUSolver::CPUSolver(std::shared_ptr<FEMContext> fem_ctx) {
    this->fem_ctx = fem_ctx;
}

bool CPUSolver::has_numerical_instability() {
    for (int i = 0; i < u.size(); i++) {
        if (u.coeff(i) > 1e4f || v.coeff(i) > 1e4f) {
            return true;
        }
    }
    return false;
}

void CPUSolver::clear_values() {
    u.resize(fem_ctx->num_unknowns());
    u.setZero();
    v.resize(fem_ctx->num_unknowns());
    v.setZero();
}

/**
 * Map unknown values from the surface to an Eigen::VectorXf.
 */
Eigen::VectorXf CPUSolver::get_surface_value_vector() {
    Eigen::VectorXf vector;
    vector.resize(fem_ctx->num_unknowns());
    for (int i = 0; i < fem_ctx->idx_map.size(); i++)
        if (fem_ctx->idx_map[i] != -1)
            vector.coeffRef(fem_ctx->idx_map[i], 0) = fem_ctx->surface->values[i];

    return vector;
}

/**
 * Map unknown values from an Eigen::VectorXf back onto the surface.
 */
void CPUSolver::map_vector_to_surface(const Eigen::VectorXf& vector) {
    for (int i = 0; i < fem_ctx->idx_map.size(); i++) {
        if (fem_ctx->idx_map[i] != -1) {
            fem_ctx->surface->values[i] = vector.coeff(fem_ctx->idx_map[i], 0);
        } else {
            fem_ctx->surface->values[i] = 0.0f;
        }
    }
}

void CPUSolver::advance_time() {
    Eigen::ConjugateGradient<Eigen::SparseMatrix<float>, Eigen::Lower|Eigen::Upper> cg;

    switch (fem_ctx->equation) {
        /**
         * Solver for the 2D Heat Equation:
         * https://en.wikipedia.org/wiki/Heat_equation
         */
        case Equation::Heat: {
            auto params = std::static_pointer_cast<HeatParameters>(fem_ctx->parameters[Equation::Heat]);

            u = get_surface_value_vector();

            Eigen::SparseMatrix<float> A = (fem_ctx->mass_matrix / params->time_step) + (params->conductivity * fem_ctx->stiffness_matrix);
            Eigen::VectorXf b = (fem_ctx->mass_matrix / params->time_step) * u;

            cg.compute(A);
            u = cg.solve(b);

            map_vector_to_surface(u);
        } break;

        /**
         * Solver for the Advection-Diffusion Equation:
         * https://en.wikipedia.org/wiki/Convection%E2%80%93diffusion_equation
         */
        case Equation::Advection_Diffusion: {
            auto params = std::static_pointer_cast<AdvectionDiffusionParameters>(fem_ctx->parameters[Equation::Advection_Diffusion]);

            u = get_surface_value_vector();

            Eigen::SparseMatrix<float> A = (fem_ctx->mass_matrix / params->time_step) + (params->c * fem_ctx->stiffness_matrix) - fem_ctx->advection_matrix;
            Eigen::VectorXf b = (fem_ctx->mass_matrix / params->time_step) * u;

            cg.compute(A);
            u = cg.solve(b);

            map_vector_to_surface(u);
        } break;

        /**
         * Solver for the 2D Wave Equation:
         * https://en.wikipedia.org/wiki/Wave_equation
         */
        case Equation::Wave: {
            auto params = std::static_pointer_cast<WaveParameters>(fem_ctx->parameters[Equation::Wave]);

            u = get_surface_value_vector();

            Eigen::SparseMatrix<float> A_v = (fem_ctx->mass_matrix / params->time_step) + (params->c * params->c * fem_ctx->stiffness_matrix * params->time_step);
            Eigen::VectorXf b_v = (fem_ctx->mass_matrix / params->time_step) * v - params->c * params->c * fem_ctx->stiffness_matrix * u;

            cg.compute(A_v);
            v = cg.solve(b_v);
            u = u + v * params->time_step;

            map_vector_to_surface(u);
        } break;

        /**
         * Solver for the 2D Gray-Scott Reaction-Diffusion Equation:
         * https://groups.csail.mit.edu/mac/projects/amorphous/GrayScott/ 
        */
        case Equation::Reaction_Diffusion: {
            auto params = std::static_pointer_cast<ReactionDiffusionParameters>(fem_ctx->parameters[Equation::Reaction_Diffusion]);

            v = get_surface_value_vector();

            // Setup for a semi-implicit time stepping scheme
            Eigen::SparseMatrix<float> A_u = fem_ctx->mass_matrix / params->time_step + params->Du * fem_ctx->stiffness_matrix;
            Eigen::VectorXf b_u = (fem_ctx->mass_matrix / params->time_step) * u - (u.cwiseProduct(v.cwiseProduct(v))) + params->feed_rate * (Eigen::VectorXf::Ones(fem_ctx->num_unknowns()) - u);

            Eigen::SparseMatrix<float> A_v = fem_ctx->mass_matrix / params->time_step + params->Dv * fem_ctx->stiffness_matrix;
            Eigen::VectorXf b_v = (fem_ctx->mass_matrix / params->time_step) * v + (u.cwiseProduct(v.cwiseProduct(v))) - (params->feed_rate + params->kill_rate) * v;

            cg.compute(A_u);
            u = cg.solve(b_u);
            cg.compute(A_v);
            v = cg.solve(b_v);

            map_vector_to_surface(v);
        } break;
    }
}