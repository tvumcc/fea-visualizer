#version 460 core
layout (location = 0) in vec3 va_position;
layout (location = 1) in vec2 va_uv;
layout (location = 2) in vec3 va_normal;

out vec2 uv;

void main() {
    uv = va_uv;
    gl_Position = vec4(va_position, 1.0);
}