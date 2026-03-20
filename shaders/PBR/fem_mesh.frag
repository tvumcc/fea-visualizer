/*
    fem_mesh.frag

    This fragment shader renders the surface of a finite element mesh using
    diffuse irradiance and specular image based lighting from the environment.
    It also discards pixels with solution values that fall below a
    client-defined threshold.
*/

#version 460 core

in float value;
in vec3 normal;
in vec3 frag_pos;

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

vec3 F_roughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    if (pixel_discard_threshold != 0.0 && value < pixel_discard_threshold - 0.001) discard;

    vec3 N = normalize(normal);
    vec3 V = normalize(view_pos - frag_pos);
    vec3 R = reflect(-V, N);

    vec3 albedo = pow(get_color(value), vec3(2.2));
    float roughness = 0.0;
    float metallic = 0.1;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

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

    vec3 ambient = kD * diffuse + specular;
    vec3 color = ambient;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}