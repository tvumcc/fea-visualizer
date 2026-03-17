#version 460 core

out vec4 FragColor;
in vec3 local_pos;

uniform samplerCube environment_map;

const float PI = 3.1415926535;

void main() {
    vec3 normal = normalize(local_pos);
    vec3 irradiance = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    float d_phi = 0.025;
    float d_theta = 0.025;
    int num_samples = 0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += d_phi) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += d_theta) {
            vec3 cartesian = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 rotated_vec = cartesian.x * right + cartesian.y * up + cartesian.z * normal;
            irradiance += texture(environment_map, rotated_vec).rgb * cos(theta) * sin(theta);
            num_samples++;
        }
    }

    irradiance = PI * irradiance / float(num_samples);
    FragColor = vec4(irradiance, 1.0);
}