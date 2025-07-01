#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aValue;

uniform mat4 model;
uniform mat4 view_proj;

out float value;

void main() {
    value = aValue;
    gl_Position = view_proj * model * vec4(aPos, 1.0);
}