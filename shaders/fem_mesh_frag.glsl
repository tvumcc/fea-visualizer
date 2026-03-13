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

vec3 get_color(float t) {
    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));
}

in float value;
in vec3 normal;
in vec3 frag_pos;

void main() {
    vec3 light_pos = vec3(3.0, 3.0, 3.0);
    vec3 light_dir = normalize(light_pos - frag_pos);
    vec3 light_color = vec3(1.0, 1.0, 1.0);

    if (pixel_discard_threshold != 0.0 && value < pixel_discard_threshold) discard;




    float ambient_strength = 0.4;
    vec3 ambient = ambient_strength * light_color;

    float diff = max(dot(normalize(normal), light_dir), 0.0);
    vec3 diffuse = diff * light_color;


    // FragColor = vec4(normalize(normal), 1.0);
    vec3 result = (ambient + diffuse) * get_color(value);
    FragColor = vec4(result, 1.0);
}