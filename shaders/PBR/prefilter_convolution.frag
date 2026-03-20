/*
    prefilter_convolution.frag

    This fragment shader precomputes the pre-filter map used for
    specular IBL.
*/

#version 460 core

in vec3 local_pos;

out vec4 FragColor;

uniform samplerCube environment_map;
uniform float roughness;

const float PI = 3.1415926535;

float D(vec3 N, vec3 H, float roughness) {
    float a2 = pow(roughness, 4);
    float NdotH2 = pow(max(dot(N, H), 0.0), 2);
    
    float numerator = a2;
    float denominator = PI * pow(NdotH2 * (a2 - 1.0) + 1.0, 2);
    return numerator / denominator;
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

void main() {
    vec3 N = normalize(local_pos);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 2048;
    float total_weight = 0.0;
    vec3 prefiltered_color = vec3(0.0);

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        vec2 lds_vec = hammersley(i, SAMPLE_COUNT);
        vec3 H = importance_sample_GGX(lds_vec, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            float NdotH = max(dot(N, H), 0.0);
            float NdotV = max(dot(N, V), 0.0);
            float pdf = D(N, H, roughness) * NdotH / (4.0 * NdotV) + 0.0001;

            float resolution = 1024.0;
            float sa_texel = 4.0 * PI / (6.0 * resolution * resolution);
            float sa_sample  = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mip_level = roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);

            prefiltered_color += textureLod(environment_map, L, mip_level).rgb * NdotL;
            total_weight += NdotL;
        }
    }

    prefiltered_color = prefiltered_color / total_weight;
    FragColor = vec4(prefiltered_color, 1.0);
}