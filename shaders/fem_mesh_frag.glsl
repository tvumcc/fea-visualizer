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

void main() {
    if (pixel_discard_threshold != 0.0 && value < pixel_discard_threshold) discard;
    // FragColor = vec4(get_color(value), 1.0);
    FragColor = vec4(normalize(normal), 1.0);
}