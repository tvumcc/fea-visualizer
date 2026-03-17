#version 330 core
out vec4 FragColor;

uniform vec3 c0;
uniform vec3 c1;
uniform vec3 c2;
uniform vec3 c3;
uniform vec3 c4;
uniform vec3 c5;
uniform vec3 c6;

uniform float pixel_discard_threshold;
uniform vec3 view_pos;

uniform samplerCube irradiance_map;
uniform samplerCube prefilter_map;
uniform sampler2D brdf_texture;

vec3 get_color(float t) {
    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));
}


in float value;
in vec3 normal;
in vec3 frag_pos;

const float PI = 3.1415926535;

float D(vec3 N, vec3 H, float roughness) {
    float a2 = pow(roughness, 4);
    float NdotH2 = pow(max(dot(N, H), 0.0), 2);
    
    float numerator = a2;
    float denominator = PI * pow(NdotH2 * (a2 - 1.0) + 1.0, 2);
    return numerator / denominator;
}

vec3 F(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 F_roughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float G_SchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;
    return numerator / denominator;
}

float G(vec3 N, vec3 V, vec3 L, float roughness) {
    return G_SchlickGGX(max(dot(N, V), 0.0), roughness) * G_SchlickGGX(max(dot(N, L), 0.0), roughness);
}

void main() {
    if (pixel_discard_threshold != 0.0 && value < pixel_discard_threshold - 0.001) discard;

    vec3 N = normalize(normal);
    vec3 V = normalize(view_pos - frag_pos);
    vec3 R = reflect(-V, N);

    vec3 light_positions[4];
    vec3 light_colors[4];
    light_positions[0] = vec3(0.0, 0.0, 0.0);
    light_positions[1] = vec3(-1.0, 1.0, -1.0);
    light_positions[2] = vec3(-1.0, 1.0, 1.0);
    light_positions[3] = vec3(1.0, 1.0, -1.0);

    light_colors[0] = 3.0 * vec3(1.0, 1.0, 1.0);
    light_colors[1] = 8.0 * vec3(1.0, 1.0, 1.0);
    light_colors[2] = 8.0 * vec3(1.0, 1.0, 1.0);
    light_colors[3] = 8.0 * vec3(1.0, 1.0, 1.0);

    vec3 albedo = pow(get_color(value), vec3(2.2));
    float roughness = 0.0;
    float metallic = 1.0; 
    float ambient_occlusion = 1.0;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < 0; i++) {
        vec3 L = normalize(light_positions[i] - frag_pos);
        vec3 H = normalize(V + L);

        float dist = length(light_positions[i] - frag_pos);
        float attenuation = (dist * dist);
        vec3 radiance = light_colors[i] / attenuation;

        vec3 fresnel = F(max(dot(H, V), 0.0), F0);

        vec3 numerator = D(N, H, roughness) * fresnel * G(N, V, L, roughness);
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = fresnel;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 fresnel = F_roughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = fresnel;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradiance_map, N).rgb;
    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefiltered_color = textureLod(prefilter_map, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf_integral = texture(brdf_texture, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefiltered_color * (fresnel * brdf_integral.x + brdf_integral.y);

    vec3 ambient = (kD * diffuse + specular) * ambient_occlusion;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}