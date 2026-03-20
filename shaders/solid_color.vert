#version 460 core
layout (location = 0) in vec3 va_position;
layout (location = 1) in vec2 va_uv;

out vec2 uv;

uniform mat4 model;
uniform mat4 view_proj;

void main() {
    uv = va_uv;
    gl_Position = view_proj * model * vec4(va_position, 1.0);
}