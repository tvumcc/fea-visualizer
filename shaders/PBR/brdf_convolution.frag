/*
    brdf_convolution.frag

    This fragment shader precomputes a 2D texture lookup table containing
    a pair of floats that stores the value of the Cook-Torrance BRDF
    integrated over a hemisphere. 
*/

#version 460 core

in vec2 uv;

out vec4 FragColor;

const float PI = 3.1415926535;

float schlick_ggx(float NdotV, float roughness) {
    float k = pow(roughness, 4) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float ggx2 = schlick_ggx(max(dot(N, V), 0.0), roughness);
    float ggx1 = schlick_ggx(max(dot(N, L), 0.0), roughness);
    return ggx1 * ggx2;
}

float van_der_corput(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), van_der_corput(i));
}

vec3 importance_sample_GGX(vec2 lds_vec, vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * lds_vec.x;
    float cos_theta = sqrt((1.0 - lds_vec.y) / (1.0 + (a * a - 1.0) * lds_vec.y));
    float sin_theta = sqrt(1.0 - pow(cos_theta, 2));

    vec3 H = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sample_vec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sample_vec);
}

vec2 integrate_brdf(float NdotV, float roughness) {
    vec3 V;
    V.x = sqrt(1.0 - pow(NdotV, 2.0));
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const int SAMPLE_COUNT = 1024;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        vec2 lds_vec = hammersley(i, SAMPLE_COUNT);
        vec3 H = importance_sample_GGX(lds_vec, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0) {
            float G = smith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        } 
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main() {
    FragColor = vec4(integrate_brdf(uv.x, uv.y), 0.0, 0.0);
}