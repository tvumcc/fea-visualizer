#pragma once
#include <Eigen/Dense>

struct EquationParameters {
    float time_step;
};

struct HeatParameters : public EquationParameters {
    float conductivity;

    HeatParameters() {
        time_step = 0.01f;
        conductivity = 0.05f;
    }
};

struct AdvectionDiffusionParameters : public EquationParameters {
    float c;
    Eigen::Vector3f velocity;

    AdvectionDiffusionParameters() {
        time_step = 0.001f;
        c = 0.25f;
        velocity = {1.0f, 0.0f, 0.0f};
    }
};

struct WaveParameters : public EquationParameters {
    float c;
    
    WaveParameters() {
        time_step = 0.05f;
        c = 0.05f;
    }
};

struct ReactionDiffusionParameters : public EquationParameters {
    float Du;
    float Dv;
    float feed_rate;
    float kill_rate;

    ReactionDiffusionParameters() {
        time_step = 0.001f;
        Du = 0.08f;
        Dv = 0.04f;
        feed_rate = 0.035f;
        kill_rate = 0.06f;
    }
};